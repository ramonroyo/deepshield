#include <stdio.h>
#include <intrin.h>
#include <cstdint>
#include <windows.h>

constexpr int BLOCK_SIZE = 16;
constexpr int PAGE_SIZE  = 4096;

#define set_process_affinity(n) SetProcessAffinityMask(GetCurrentProcess(), n)

uint64_t timming_rdtsc[BLOCK_SIZE];
uint64_t timming_rdpmc_host_tsc[BLOCK_SIZE];
uint64_t timming_rdpmc_elapsed_ns[BLOCK_SIZE];
uint64_t timming_rdpmc_virtual_ns[BLOCK_SIZE];

extern "C" {
  uint64_t __vmware_tsc();
  bool detect_vmware();
  uint64_t memory_access_time(void *p);
  uint64_t rdpmc_memory_access(void *p, unsigned int profile);
}

void dump_times(uint64_t * times, int size, char* tag) {
    printf("############## %s ##############\n", tag);
    for (int i = 0; i < size; ++i) {
      if ((i + 1) % 16 == 0)
        printf("% 6llu\n", times[i]);
      else
        printf("% 6llu", times[i]);
    }
    printf("##################################\n");
}


class Pool {
  uint8_t * memory;
public:
  Pool(void);
  ~Pool(void);
  void flush(void);
  void flush_odd(void);
  uint8_t * ptr(void);
  uint64_t touch(unsigned int offset);
};

uint8_t * Pool::ptr(void) {
    return memory;
}

uint64_t Pool::touch(unsigned int index) {
    return memory_access_time(ptr() + index * PAGE_SIZE );
}

void Pool::flush(void) {
  for (int i = 0; i < BLOCK_SIZE; ++i) {
    _mm_clflush(memory + i * PAGE_SIZE);
  }
}

void Pool::flush_odd(void) {
  for (int i = 1; i < BLOCK_SIZE; i += 2) {
    _mm_clflush(memory + i * PAGE_SIZE);
  }
}


Pool::Pool() {
  memory = (uint8_t*) malloc(BLOCK_SIZE * PAGE_SIZE);
  memset(memory, 0xfe, sizeof(memory));
}

Pool::~Pool() {
  if ( memory ) {
    free( memory );
  }
}

uint64_t measure_vmware_cpuid() {
  uint64_t before;
  uint64_t after;

  int cpu_info[4];
  int function_id = 0;

  before = __vmware_tsc();
  __cpuid(cpu_info, function_id);
  after = __vmware_tsc();

  return (after - before);
}

uint64_t measure_cpuid() {
  uint64_t before;
  uint64_t after;

  int cpu_info[4];
  int function_id = 0;

  before = __rdtsc();
  __cpuid(cpu_info, function_id);
  after = __rdtsc();

  return (after - before);
}

uint64_t measure_vmware_rdtsc() {
  uint64_t before;
  uint64_t after;

  before = __vmware_tsc();
  __rdtsc();
  after = __vmware_tsc();

  return (after - before);
}

uint64_t measure_vmware_rdtscp() {
  uint64_t before;
  uint64_t after;
  unsigned int processor;

  before = __vmware_tsc();
  __rdtscp(&processor);
  after = __vmware_tsc();

  return (after - before);
}


uint64_t measure_rdtsc() {
  uint64_t before;
  uint64_t after;

  before = __rdtsc();
  __rdtsc();
  after = __rdtsc();

  return (after - before);
}

uint64_t measure_rdtscp() {
  uint64_t before;
  uint64_t after;
  unsigned int processor;

  before = __rdtsc();
  __rdtscp(&processor);
  after = __rdtsc();

  return (after - before);
}

void measure_instruction(uint64_t (*callback)(void), char * name) {
    uint64_t average = 0;
    uint64_t min = 0xFFFFFFFFFFFFFFFF;
    uint64_t max = 0;
    uint64_t time;
    int skips = 0;

    printf("\nmeasuring ** %s ** instruction with native TSC\n", name);
    printf("===========================================\n");

    for ( int i = 0; i < 50000; i++ ) {

        time = callback();

        if ( time > max ) { max = time; }
        if ( time < min ) { min = time; }

        if ( (i > 0) && (time > (average * 3)) ) {
            skips++;
            continue;
        }

        average += time;
        if ( i > 0 ) {
            average /= 2;
        }
    }

    printf("average of %s: %llu | min: %llu | max %llu (skips %d)\n",
              name,
              average,
              min,
              max,
              skips);

}

void start_tsc_measure(bool is_vmware) {

  Pool pool = Pool::Pool();


  for (int i=0; i < BLOCK_SIZE; i++) {
    pool.touch(i);
  }

  for (int trials=0; trials < 500; trials++ ) {

      for (int i = 0; i < BLOCK_SIZE; ++i) {
        void * address = pool.ptr() + i * 4096;

        if ( is_vmware ) {

          if (i % 2 != 0) {
            _mm_clflush(address);
          }
          timming_rdpmc_host_tsc[i]   += rdpmc_memory_access(address, 0x10000);

          if (i % 2 != 0) {
            _mm_clflush(address);
          }
          timming_rdpmc_elapsed_ns[i] += rdpmc_memory_access(address, 0x10001);

          if (i % 2 != 0) {
            _mm_clflush(address);
          }
          timming_rdpmc_virtual_ns[i] += rdpmc_memory_access(address, 0x10002);

          if ( trials > 0 ) {
              timming_rdpmc_host_tsc[i]   /= 2;
              timming_rdpmc_elapsed_ns[i] /= 2;
              timming_rdpmc_virtual_ns[i] /= 2;
          }
        }

        if (i % 2 != 0) {
          _mm_clflush(address);
        }

        timming_rdtsc[i] += memory_access_time(address);

        if ( trials > 0 ) {
          timming_rdtsc[i] /= 2;
        }
      }
  }

  if ( is_vmware ) {
    dump_times(timming_rdpmc_host_tsc,    BLOCK_SIZE, "RDPMC (Native TSC)\0");
    dump_times(timming_rdpmc_elapsed_ns,  BLOCK_SIZE, "RDPMC (Elapsed NS)\0");
    dump_times(timming_rdpmc_virtual_ns,  BLOCK_SIZE, "RDPMC (Apparent NS)\0");
  }

  dump_times(timming_rdtsc, BLOCK_SIZE, "RDTSC\0");

  if ( is_vmware ) {
    measure_instruction(measure_vmware_rdtsc,  "VMWARE-RDTSC\0");
    measure_instruction(measure_vmware_rdtscp, "VMWARE-RDTSC(P)\0");
    measure_instruction(measure_vmware_cpuid,  "VMWARE-CPUID\0");
  } else {
    measure_instruction(measure_rdtsc,  "RDTSC\0");
    measure_instruction(measure_rdtscp, "RDTSC(P)\0");
    measure_instruction(measure_cpuid,  "CPUID\0");
  }
}

int main(int argc, char *argv[]) {
  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  bool is_vmware = false;

  __try {
      is_vmware = detect_vmware();
  }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      is_vmware = false;
  }


  if ( is_vmware ) {
    printf("VMWARE is detected, using physical TSC.\n");
  } else {
    printf("VMWARE is NOT detected, using native TSC.\n");
  }

  system("pause");
  set_process_affinity(1);
  start_tsc_measure(is_vmware);
  return 0;
}
