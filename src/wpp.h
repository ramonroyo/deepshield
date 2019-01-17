/*++

Copyright (c) ByteHeed. All rights reserved.

Module Name:
    
    wpp.h

Abstract:
    
    Header file for the debug tracing related function defintions and macros.

Environment:
    
    Kernel mode

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <evntrace.h> // For TRACE_LEVEL definitions

#if defined(WPP_EVENT_TRACING)

#define WPP_CHECK_FOR_NULL_STRING  // to prevent exceptions due to NULL strings.

//
// WPP_DEFINE_CONTROL_GUID specifies the GUID used for DeepShield driver.
// {F44578FD-73A4-4D8A-A876-109D8A677F4E}
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(DsTraceGuid,(f44578fd, 73a4, 4d8a, a876, 109d8a677f4e), \
        WPP_DEFINE_BIT(TRACE_FUNC)                \
        WPP_DEFINE_BIT(TRACE_INIT)                \
        WPP_DEFINE_BIT(TRACE_CREATE_CLOSE)        \
        WPP_DEFINE_BIT(TRACE_IOCTL)               \
        WPP_DEFINE_BIT(TRACE_WMI)                 \
        WPP_DEFINE_BIT(TRACE_DEVICE)              \
        WPP_DEFINE_BIT(TRACE_IOA)                 \
        WPP_DEFINE_BIT(TRACE_IOA_RSVD)            \
        WPP_DEFINE_BIT(TRACE_MSR)                 \
        WPP_DEFINE_BIT(TRACE_MSR_RSVD)            \
        WPP_DEFINE_BIT(TRACE_NOTIFICATION)        \
        WPP_DEFINE_BIT(TRACE_CHANNEL)             \
        WPP_DEFINE_BIT(TRACE_MAILBOX)             \
        WPP_DEFINE_BIT(TRACE_RINGBUF)             \
        WPP_DEFINE_BIT(TRACE_PROCESS)             \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

// This comment block is scanned by the trace preprocessor to define our
// Trace functions.
//
// begin_wpp config

// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// FUNC TraceFatal{LEVEL=TRACE_LEVEL_FATAL}(FLAGS, MSG, ...);
// FUNC TraceError{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG, ...);
// FUNC TraceWarning{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
// FUNC TraceAttacks{FLAG=TRACE_IOA}(LEVEL, MSG, ...);
//
// end_wpp

#define TRACE_IOA_ROOT         0x00000080
#define TRACE_MSR_ROOT         0x00000200

#else

#if !defined(TRACE_LEVEL_NONE)
#define TRACE_LEVEL_NONE        0
#define TRACE_LEVEL_CRITICAL    1
#define TRACE_LEVEL_FATAL       1
#define TRACE_LEVEL_ERROR       2
#define TRACE_LEVEL_WARNING     3
#define TRACE_LEVEL_INFORMATION 4
#define TRACE_LEVEL_VERBOSE     5
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9
#endif

//
// Define Debug Flags.
//

#define TRACE_FUNC               0x00000001
#define TRACE_INIT               0x00000002
#define TRACE_CREATE_CLOSE       0x00000004
#define TRACE_IOCTL              0x00000008
#define TRACE_WMI                0x00000010
#define TRACE_DEVICE             0x00000020
#define TRACE_IOA                0x00000040
#define TRACE_IOA_ROOT           0x00000080
#define TRACE_MSR                0x00000100
#define TRACE_MSR_ROOT           0x00000200
#define TRACE_NOTIFICATION       0x00000400
#define TRACE_CHANNEL            0x00000800
#define TRACE_MAILBOX            0x00001000
#define TRACE_RINGBUF            0x00002000
#define TRACE_PROCESS            0x00004000


VOID
TraceEvents(
    _In_ ULONG DebugPrintLevel,
    _In_ ULONG DebugPrintFlag,
    _In_z_ __drv_formatString( printf ) PCSTR DebugMessage,
    ...
    );

#define WPP_INIT_TRACING(DriverObject, RegistryPath)
#define WPP_CLEANUP(DriverObject)

#endif

//
// Friendly names for various trace levels.
//

typedef enum _INTERNAL_TRACE_LEVELS {
    ERROR = TRACE_LEVEL_ERROR,
    WARN = TRACE_LEVEL_WARNING,
    INFO = TRACE_LEVEL_INFORMATION,
    VERBOSE = TRACE_LEVEL_VERBOSE
} INTERNAL_TRACE_LEVELS, *PINTERNAL_TRACE_LEVELS;

#define DebugAssert(_exp) NT_ASSERT(_exp)

#ifdef __cplusplus
}
#endif