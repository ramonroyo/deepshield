# DeepShield Hypervisor

DeepShield is a state-of-the-art type-2 hypervisor for Intel processors to enhance security through mitigating side channel and timing attacks.

## Why DeepShield?

Side channel and timing attacks, while infrequent, pose severe risks in cybersecurity. DeepShield mitigates these tail risks by deprivileging key instructions, protecting against the potential exposure of sensitive data and system breaches that can arise from these precise, time-sensitive exploits.

## Background

DeepShield took shape under the umbrella of ByteHeed's MemoryGuard’s ambitious endeavor, a hypervisor-driven security mechanisms providing byte-level interception and transparent hooking underneath the OS. While MemoryGuard reached its development goals, the path for ByteHeed startup ended earlier than founders hoped 4 years ago. DeepShield has been made public now, reflecting the spirit of what was achieved and as a testament to its innovative aspirations. Should the research community show interest, the future may reveal more of the MemoryGuard story, potentially leading to collaborative exploration and expansion.

## Features

- **Timing Attack Mitigation:** [Deprivileges RDTSC/P instructions](https://github.com/ramonroyo/deepshield/blob/master/src/vmcs.c#L245) with TSD bit to prevent precise timing measurements by unauthorized code.

- **Memory Access Inspection:** [Checks guest memory accesses](https://github.com/ramonroyo/deepshield/blob/master/src/exits.c#L195) happening between consecutive pairs of RDTSC/P readings to detect potential [attack patterns](https://github.com/ramonroyo/deepshield/blob/master/src/tsc.c#L590), and reinject the TSD trigered exceptions.

- **Transparent Execution** Leverages advanced capabilities of Intel's virtualization technology to ensure transparent protection and [CR4 emulation](https://github.com/ramonroyo/deepshield/blob/master/src/instr.c#L174)

- **Memory Guard Spin-Off:** Builds upon the groundwork laid by MemoryGuard, which offered a sophisticated SDK for byte-level hooking at sensitive and system-critical points, utilizing Windows symbols for precise interception of execution and data access.

## Getting Started

The DeepShield hypervisor is a core component of an extensive project that includes multiple modules: service, tray-app, updater, tester, etc. As of now, we are only making the hypervisor part publicly available. This release is intended primarily for learning and educational purposes, to demonstrate the use of Intel VT-d extensions for security and to serve as a practical example of mitigating side-channel and timing attacks. Long story short, we decided not to release the entire suite at this time.
