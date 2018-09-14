/**
*  @file    x86.h
*  @brief   x86 architecture related functions
*
*  Provides functions that are closely related with x86 architecture in form of intrinsics lookalike.
*/
#ifndef __X86_H__
#define __X86_H__

#include <ntifs.h>

#pragma warning(disable:4201)
#pragma warning(disable:4214)

#define IA32_TSC                0x10
#define IA32_APIC_BASE          0x1B
#define IA32_TSC_ADJUST         0x3B
#define IA32_SYSENTER_CS        0x174
#define IA32_SYSENTER_ESP       0x175
#define IA32_SYSENTER_EIP       0x176
#define IA32_DEBUGCTL           0x1D9
#define IA32_PERF_GLOBAL_CTRL   0x38F
#define IA32_FS_BASE            0xC0000100
#define IA32_GS_BASE            0xC0000101
#define IA32_GS_BASE_SWAP       0xC0000102
#define IA32_TSC_AUX            0xC0000103
#define IA32_MSR_EFER           0xC0000080

#define RFLAGS_CF   1u
#define RFLAGS_ZF   (1u << 6)
#define RFLAGS_TF   (1u << 8)
#define RFLAGS_IF   (1u << 9)
#define RFLAGS_DF   (1u << 10)
#define RFLAGS_IOPL (3u << 12)
#define RFLAGS_NT   (1u << 14)
#define RFLAGS_RF   (1u << 16)
#define RFLAGS_VM   (1u << 17)
#define RFLAGS_AC   (1u << 18)
#define RFLAGS_VIF  (1u << 19)
#define RFLAGS_VIP  (1u << 20)

//
//  IA-32 Control Register #0 (CR0)
//

#define CR0_PE    1u
#define CR0_TS    (1u << 3)
#define CR0_NE    (1u << 5)
#define CR0_WP    (1u << 16)
#define CR0_NW    (1u << 29)
#define CR0_CD    (1u << 30)
#define CR0_PG    (1u << 31)

//
//  IA-32 Control Register #0 (CR4)
//

#define CR4_VME          1u
#define CR4_TSD          (1u << 2)
#define CR4_PSE          (1u << 4)
#define CR4_PAE          (1u << 5)
#define CR4_PGE          (1u << 7)
#define CR4_OSFXSR       (1u << 9)
#define CR4_OSXMMEXCPT   (1u << 10)
#define CR4_VMXE         (1u << 13)
#define CR4_SMXE         (1u << 14)
#define CR4_OSXSAVE      (1u << 18)

#define CPUID_BASIC_INFORMATION                   0x0
#define CPUID_FEATURE_INFORMATION                 0x1
#define CPUID_PROCESSOR_EXTENDED_STATE_EMULATION  0xD
#define CPUID_EXTENDED_INFORMATION                0x80000000
#define CPUID_EXTENDED_ADDRESS_SIZE               0x80000008

#pragma warning( disable : 4214 ) //Bit field types other than int

typedef union _FLAGS_REGISTER
{
    struct
    {
        UINT32 cf         : 1;   // 0 - Carry flag
        UINT32 _reserved0 : 1;   // 1 - Always 1
        UINT32 pf         : 1;   // 2 - Parity flag
        UINT32 _reserved1 : 1;   // 3 - Always 0
        UINT32 af         : 1;   // 4 - Borrow flag
        UINT32 _reserved2 : 1;   // 5 - Always 0
        UINT32 zf         : 1;   // 6 - Zero flag
        UINT32 sf         : 1;   // 7 - Sign flag
        UINT32 tf         : 1;   // 8 - Trap flag
        UINT32 intf       : 1;   // 9 - Interrupt flag
        UINT32 df         : 1;   // 10 - Direction flag
        UINT32 of         : 1;   // 11 - Overflow flag
        UINT32 iopl       : 2;   // 12 - 13 - I/O privilege level
        UINT32 nt         : 1;   // 14 - Nested task flag
        UINT32 _reserved3 : 1;   // 15 - Always 0
        UINT32 rf         : 1;   // 16 - Resume flag
        UINT32 vm         : 1;   // 17 - Virtual 8086 mode
        UINT32 ac         : 1;   // 18 - Alignment check
        UINT32 vif        : 1;   // 19 - Virtual interrupt flag
        UINT32 vip        : 1;   // 20 - Virtual interrupt pending
        UINT32 id         : 1;   // 21 - Identification flag
        UINT32 _reserved4 : 10;  // 22 - 31 - Always 0
#ifdef _WIN64
        UINT32 _zero0     : 32;  // 32 - 63 - Must be zero od #PG
#endif
    } Bits;

    UINTN AsUintN;
} FLAGS_REGISTER;

typedef union _CR0_REGISTER
{
    struct
    {
        UINT32 pe         : 1;   // 0 - Protected Mode Enabled
        UINT32 mp         : 1;   // 1 - Monitor Coprocessor
        UINT32 em         : 1;   // 2 - Emulate
        UINT32 ts         : 1;   // 3 - Task Switched
        UINT32 et         : 1;   // 4 - Extension Type
        UINT32 ne         : 1;   // 5 - Numeric Error
        UINT32 _reserved0 : 10;  // 6 - 15
        UINT32 wp         : 1;   // 16 - Write Protect
        UINT32 _reserved1 : 1;   // 17
        UINT32 am         : 1;   // 18 - Alignment Mask
        UINT32 _reserved2 : 10;  // 19 - 28
        UINT32 nw         : 1;   // 29 - Not Write-Through
        UINT32 cd         : 1;   // 30 - Cache Disable
        UINT32 pg         : 1;   // 31 - Paging Enabled
#ifdef _WIN64
        UINT32 _zero0     : 32;  // 32 - 63 - Must be zero od #PG
#endif
    } Bits;

    UINTN AsUintN;
} CR0_REGISTER, VMX_MSR_CR0;

typedef union _CR3_REGISTER
{
    struct
    {
        UINTN Rsvd0To2   : 3; // 0 - 2
        UINTN PWT        : 1; // 3
        UINTN PCD        : 1; // 4
        UINTN _reserved1 : 7; // 5 - 11
#ifndef _WIN64
        UINTN PDB        : 20; // 12 - 31
#else
        UINTN PDB        : 52; // 12 - 63
#endif
    } Bits;

    UINTN AsUintN;
} CR3_REGISTER;

typedef union _CR4_REGISTER
{
    struct
    {
        UINT32 vme        : 1;  // 0 - Virtual Mode Extensions
        UINT32 pvi        : 1;  // 1 - Protected-Mode Virtual Interrupts
        UINT32 Tsd        : 1;  // 2 - Time Stamp Disable
        UINT32 de         : 1;  // 3 - Debugging Extensions
        UINT32 pse        : 1;  // 4 - Page Size Extensions
        UINT32 pae        : 1;  // 5 - Physical Address Extension
        UINT32 mce        : 1;  // 6 - Machine-Check Enable
        UINT32 pge        : 1;  // 7 - Page Global Enable
        UINT32 pce        : 1;  // 8 - Performance-Monitoring Counter Enable
        UINT32 osfxsr     : 1;  // 9 - OS Support for FXSAVE/FXRSTOR
        UINT32 osxmmexcpt : 1;  // 10 - OS Support for Unmasked SIMD Exceptions
        UINT32 umip       : 1;  // 11 - User mode instruction prevention
        UINT32 _reserved0 : 1;  // 12
        UINT32 vmxe       : 1;  // 13 - Virtual Machine Extensions Enabled
        UINT32 smxe       : 1;  // 14 - SMX-Enable Bit
        UINT32 _reserved1 : 1;  // 15
        UINT32 fsgsbase   : 1;  // 16 - FS/GS R/W Instructions enable
        UINT32 pcide      : 1;  // 17 - PCID Enable
        UINT32 osxsave    : 1;  // 18 - XSAVE and Processor Extended States-Enable
        UINT32 _reserved2 : 1;  // 19
        UINT32 smep       : 1;  // 20 - Supervisor Mode Execution Protection Enable
        UINT32 smap       : 1;  // 21 - Supervisor Mode Access Protection Enable
        UINT32 pke        : 1;  // 22 - Protection Key Enable
        UINT32 _reserved3 : 9;  // 23 - 31
#ifdef _WIN64
        UINT32 _zero0     : 32; // 32 - 63 - Must be zero od #PG
#endif
    } Bits;

    UINTN AsUintN;
} CR4_REGISTER, VMX_MSR_CR4;

typedef struct _DR7_REGISTER
{
    union
    {
        UINTN raw;

        struct
        {
            UINT32 L0     : 1;  // 0 - Local Breakpoint Enable 0
            UINT32 G0     : 1;  // 1 - Global Breakpoint Enable 0
            UINT32 L1     : 1;  // 2 - Local Breakpoint Enable 1
            UINT32 G1     : 1;  // 3 - Global Breakpoint Enable 1
            UINT32 L2     : 1;  // 4 - Local Breakpoint Enable 2
            UINT32 G2     : 1;  // 5 - Global Breakpoint Enable 2
            UINT32 L3     : 1;  // 6 - Local Breakpoint Enable 3
            UINT32 G3     : 1;  // 7 - Global Breakpoint Enable 3
            UINT32 LE     : 1;  // 8 - Local Exact Breakpoint Enable
            UINT32 GE     : 1;  // 9 - Global Exact Breakpoint Enable
            UINT32 _one0  : 1;  // 10 - Always 1
            UINT32 RTM    : 1;  // 11 - Restricted Transactional Memory
            UINT32 _zero0 : 1;  // 12 - Always zero
            UINT32 GD     : 1;  // 13 - General Detect Enable
            UINT32 _zero1 : 2;  // 14 - 15 - Always zero
            UINT32 RW0    : 2;  // 16 - 17 - Condition BP0
            UINT32 LEN0   : 2;  // 18 - 19 - Length BP0
            UINT32 RW1    : 2;  // 20 - 21 - Condition BP1
            UINT32 LEN1   : 2;  // 22 - 23 - Length BP1
            UINT32 RW2    : 2;  // 24 - 25 - Condition BP2
            UINT32 LEN2   : 2;  // 26 - 27 - Length BP2
            UINT32 RW3    : 2;  // 28 - 29 - Condition BP3
            UINT32 LEN3   : 2;  // 30 - 31 - Length BP3
#ifdef _WIN64
            UINT32 _zero2 : 32;  // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} DR7_REGISTER;


typedef struct _DR6_REGISTER
{
    union
    {
        UINTN raw;

        struct
        {
            UINT32 B0     : 1;  // 0 - Breakpoint Condition Detected 0
            UINT32 B1     : 1;  // 1 - Breakpoint Condition Detected 1
            UINT32 B2     : 1;  // 2 - Breakpoint Condition Detected 2
            UINT32 B3     : 1;  // 3 - Breakpoint Condition Detected 3
            UINT32 _one0  : 8;  // 4 - 11 - Always 1
            UINT32 _zero0 : 1;  // 12 - Always 0
            UINT32 BD     : 1;  // 13 - DR access detected
            UINT32 BS     : 1;  // 14 - Single step
            UINT32 BT     : 1;  // 15 - Task switch
            UINT32 RTM    : 1;  // 16 - Restricted transactional memory
            UINT32 _one1  : 15; // 17 - 31 - Always 1
#ifdef _WIN64
            UINT32 _zero1 : 32; // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} DR6_REGISTER;


typedef struct _IDT_ENTRY
{
    union
    {
        UINT64 raw;

        struct
        {
            UINT32 offsetLow    : 16;
            UINT32 selector     : 16;
#ifdef _WIN64
            UINT32 ist          : 3;
            UINT32 _reserved0   : 5;
#else
            UINT32 _reserved0   : 8;
#endif
            UINT32 type         : 5;
            UINT32 dpl          : 2;
            UINT32 present      : 1;
            UINT32 offsetHigh   : 16;
        } f;
    } u;
#ifdef _WIN64
    UINT32    offsetUpper;
    UINT32    _reserved0;
#endif
} IDT_ENTRY, *PIDT_ENTRY;


//
//  Byte packed structure for an IDTR, GDTR, LDTR descriptor.
//
#pragma pack(push, 1)
typedef struct _IA32_DESCRIPTOR {
    UINT16 Limit;
    UINTN Base;
} IA32_DESCRIPTOR;
#pragma pack(pop)

typedef union _IA32_SEGMENT_SELECTOR
{
    struct 
    {
        UINT32 Rpl : 2;
        UINT32 Ti : 1;
        UINT32 Index : 13;
    } Bits;

    UINT16 AsUint16;
} IA32_SEGMENT_SELECTOR, *PIA32_SEGMENT_SELECTOR;

//
//  Byte packed structure for a segment descriptor in a GDT / LDT,
//
typedef union _IA32_SEGMENT_DESCRIPTOR
{
    struct 
    {
        UINT16 LimitLow;
        UINT16 BaseLow;

        union
        {
            struct
            {
                UINT8 BaseMiddle;
                UINT8 Flags1;
                UINT8 Flags2;
                UINT8 BaseHigh;
            } Bytes;

            struct
            {
                UINT32 BaseMid : 8;
                UINT32 Type : 4;
                UINT32 S : 1;
                UINT32 Dpl : 2;
                UINT32 P : 1;
                UINT32 LimitHigh : 4;
                UINT32 Avl : 1;
                UINT32 L : 1;
                UINT32 Db : 1;
                UINT32 Gran : 1;
                UINT32 BaseHigh : 8;
            } Bits;
        };

        UINT32 BaseUpper;
        UINT32 MustBeZero;
    };

    struct
    {
        INT64 DataLow;
        INT64 DataHigh;
    };
    
} IA32_SEGMENT_DESCRIPTOR, *PIA32_SEGMENT_DESCRIPTOR;


/**
* Installs a new handler in a given IDT.
*
* @param idt     [in]      Address of an IDT.
* @param service [in]      Number of service to install.
* @param handler [in]      Service handler.
* @param old     [out opt] Previous Service handler.
*/
VOID
IdtInstallService(
    _In_      UINTN   idt,
    _In_      UINT8      service,
    _In_      UINTN   handler,
    _Out_opt_ PIDT_ENTRY old
);

/**
* Gets an entry from an IDT given its number.
*
* @param idt     [in] IDT table.
* @param service [in] Service number.
* @return The address of the interrupt descriptor.
*/

PIDT_ENTRY
IdtGetEntryFromServiceNumber(
    _In_ PVOID  idt,
    _In_ UINT32 service
);

/**
* Reads the virtual address of an interrupt service.
*
* @param entry [in] IDT entry.
* @return The address of the interrupt routine.
*/
UINTN
IdtReadServiceAddressFromEntry(
    _In_ PIDT_ENTRY entry
);

/**
* Restores a previous full entry.
*
* @param idt     [in] Address of an IDT.
* @param service [in] Number of service to install.
* @param old     [in] Previous Service handler.
*/
VOID
IdtRestoreEntry(
    _In_ UINTN   idt,
    _In_ UINT8      service,
    _In_ PIDT_ENTRY old
);

/**
* Segment selector reading.
*/
UINT16 AsmReadCs(VOID);
UINT16 AsmReadSs(VOID);
UINT16 AsmReadDs(VOID);
UINT16 AsmReadEs(VOID);
UINT16 AsmReadFs(VOID);
UINT16 AsmReadGs(VOID);
UINT16 AsmReadTr(VOID);
UINT16 AsmReadLdtr(VOID);
VOID AsmReadGdtr(_Out_ IA32_DESCRIPTOR * Gdtr);


/**
* Segment selector writing.
* Almost all receive a 16-bit-wide value containing the selector.
*/
VOID __stdcall AsmWriteSs(_In_ UINT16 selector);
VOID __stdcall AsmWriteDs(_In_ UINT16 selector);
VOID __stdcall AsmWriteEs(_In_ UINT16 selector);
VOID __stdcall AsmWriteFs(_In_ UINT16 selector);
VOID __stdcall AsmWriteGs(_In_ UINT16 selector);
VOID __stdcall AsmWriteTr(_In_ UINT16 selector);
VOID __stdcall AsmWriteGdtr(_In_ IA32_DESCRIPTOR* Gdtr);

/**
* Segment descriptor reading.
* Given a selector, it is indexed in the appropiate table (GDT/LDT) and 
* different attributes are read. 
* Those attributes are base, limit, and access rights of the descriptor.
*/
UINTN BaseFromSelector (_In_ UINT16 selector);
UINT32 __stdcall AsmLimitFromSelector(_In_ UINT16 selector);
UINT32 ArFromSelector(_In_ UINT16 selector);

/**
* GDT register related functions.
* Reading is done separately (one function to obtain the base and another to get limit).
* Writing is done with a single function (entire register is modified).
*/
UINTN GetGdtrBase(VOID);
UINT16 GetGdtrLimit(VOID);

/**
* IDT register related functions.
* Reading is done separately (one function to obtain the base and another to get limit).
* Writing is done with a single function (entire register is modified).
*/
UINTN GetIdtrBase(VOID);
UINT16 GetIdtrLimit(VOID);
VOID WriteIdtr(_In_ IA32_DESCRIPTOR Idt);

/**
* Retrieve the value of the stack pointer (esp/rsp).
*/
UINTN __stdcall AsmReadSp(VOID);

/**
* Retrieve the value of the flags register (eflags/rflags).
*/
UINTN __stdcall AsmReadFlags(VOID);

/**
* Write to cr2 (page-fault address recording register).
*/
VOID __stdcall AsmWriteCr2(_In_ UINTN cr2);


/**
* Read/Write from/to cr/dr registers
*/
UINTN readcr (_In_ UINT32 cr);

_IRQL_raises_(value)
VOID     writecr(_In_ UINT32 cr, _In_ UINTN value);

UINTN readdr (_In_ UINT32 dr);
VOID     writedr(_In_ UINT32 dr, _In_ UINTN value);

/**
* cli and sti instructions instrinsics.
* Inhibit and habilitate interrupts in the executing Vcpu.
*/
VOID AsmDisableInterrupts(VOID);
VOID AsmEnableInterrupts(VOID);

/**
* Pause instruction instrinsic.
* Used in synchonization bucles.
*/
VOID AsmCpuPause(VOID);

#endif
