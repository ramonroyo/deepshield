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

Pool::Pool() {
  memory = (uint8_t*) malloc(BLOCK_SIZE * PAGE_SIZE);
  memset(memory, 0xfe, sizeof(memory));
}

Pool::~Pool() {
  if ( memory ) {
    free( memory );
  }
}

void start_tsc_measure(bool is_vmware) {

  Pool pool = Pool::Pool();


  for (int trials=0; trials < 500; trials++ ) {
      pool.flush();

      //
      // touching pair pages to fastly perceive differences
      //
      for (int i = 0; i < BLOCK_SIZE; i += 2) {
        pool.touch(i);
      }

      for (int i = 0; i < BLOCK_SIZE; ++i) {
        void * address = pool.ptr() + i * 4096;

        if ( is_vmware ) {

          timming_rdpmc_host_tsc[i]   += rdpmc_memory_access(address, 0x10000);
          timming_rdpmc_elapsed_ns[i] += rdpmc_memory_access(address, 0x10001);
          timming_rdpmc_virtual_ns[i] += rdpmc_memory_access(address, 0x10002);

          if ( trials > 0 ) {
              timming_rdpmc_host_tsc[i]   /= 2;
              timming_rdpmc_elapsed_ns[i] /= 2;
              timming_rdpmc_virtual_ns[i] /= 2;
          }
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
