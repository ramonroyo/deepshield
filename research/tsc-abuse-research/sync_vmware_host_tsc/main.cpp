#include <stdio.h>
#include <intrin.h>
#include <cstdint>
#include <windows.h>

#define set_process_affinity(n) SetProcessAffinityMask(GetCurrentProcess(), n)

extern "C" {
  uint64_t __vmware_tsc();
  bool detect_vmware();
}

void start_tsc_polling(bool is_vmware) {
  uint64_t time;

  while (1) {
    if ( is_vmware ) {
      time = __vmware_tsc();
    } else {
      time = __rdtsc();
    }

    printf("%llu\n", time);

    Sleep(1000);
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

    __try {
      __vmware_tsc();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      printf("Pseudo performance counters are not enabled, please edit\n");
      printf("your .vmx and add following lines:\n");
      printf("monitor_control.pseudo_perfctr = TRUE");
    }
  } else {
    printf("VMWARE is NOT detected, using native TSC.\n");
  }

  system("pause");
  set_process_affinity(1);
  start_tsc_polling(is_vmware);
  return 0;
}
