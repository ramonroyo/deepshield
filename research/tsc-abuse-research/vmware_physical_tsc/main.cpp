#include <stdio.h>
#include <intrin.h>
#include <cstdint>
#include <windows.h>

constexpr int BLOCK_SIZE = 256;

#define set_process_affinity(n) SetProcessAffinityMask(GetCurrentProcess(), n)

uint64_t timming_rdtsc[BLOCK_SIZE];
uint64_t timming_rdpmc_host_tsc[BLOCK_SIZE];
uint64_t timming_rdpmc_elapsed_ns[BLOCK_SIZE];
uint64_t timming_rdpmc_virtual_ns[BLOCK_SIZE];

uint8_t probe[BLOCK_SIZE * 4096];

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

void timing_research(bool is_vmware) {
  memset(probe, 0xfe, sizeof(probe));

  for (int i = 0; i < 256; ++i)
    _mm_clflush(probe + i * 4096);

  // caching half of blocks
  for (int i = 0; i < 256; i += 2)
    uint64_t _ = memory_access_time(probe + i * 4096);

  for (int i = 0; i < BLOCK_SIZE; ++i) {
    void * address = probe + i * 4096;

    if ( is_vmware ) {
      timming_rdpmc_host_tsc[i]   = rdpmc_memory_access(address, 0x10000);
      timming_rdpmc_elapsed_ns[i] = rdpmc_memory_access(address, 0x10001);
      timming_rdpmc_virtual_ns[i] = rdpmc_memory_access(address, 0x10002);
    }

    timming_rdtsc[i] = memory_access_time(address);
  }

  if ( is_vmware ) {
    dump_times(timming_rdpmc_host_tsc,   BLOCK_SIZE, "RDPMC (Native TSC)\0");
    dump_times(timming_rdpmc_elapsed_ns, BLOCK_SIZE, "RDPMC (Elapsed NS)\0");
    dump_times(timming_rdpmc_virtual_ns, BLOCK_SIZE, "RDPMC (Apparent NS)\0");
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
  timing_research(is_vmware);
  return 0;
}
