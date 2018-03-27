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
  uint64_t vmware_current;
  uint64_t vmware_last = 0;
  uint64_t current;
  uint64_t last = 0;

  while (1) {
    if ( is_vmware ) {
      vmware_current = __vmware_tsc();
      printf("%016llx (rdpmc) - diff [%llx]\n", vmware_current, (vmware_current - vmware_last));
      vmware_last = vmware_current;
    }

    current = __rdtsc();
    printf("%016llx (rdtsc) - diff [%llx]\n", current, (current - last));
    last = current;

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
