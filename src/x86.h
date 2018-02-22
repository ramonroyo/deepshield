/**
*  @file    x86.h
*  @brief    x86 architecture related functions
*
*  Provides functions that are closely related with x86 architecture in form of intrinsics lookalike.
*/
#ifndef __X86_H__
#define __X86_H__

#ifndef LYNX_USER_MODE_DBG
#include <ntddk.h>
#endif


#define IA32_SYSENTER_CS        0x174
#define IA32_SYSENTER_ESP        0x175
#define IA32_SYSENTER_EIP        0x176
#define IA32_DEBUGCTL            0x1D9
#define IA32_FS_BASE            0xC0000100
#define IA32_GS_BASE            0xC0000101
#define IA32_GS_BASE_SWAP       0xC0000102
#define IA32_APIC_BASE          0x1B


#pragma warning( disable : 4214 ) //Bit field types other than int

typedef struct _FLAGS_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned cf         : 1;   // 0 - Carry flag
            unsigned _reserved0 : 1;   // 1 - Always 1
            unsigned pf         : 1;   // 2 - Parity flag
            unsigned _reserved1 : 1;   // 3 - Always 0
            unsigned af         : 1;   // 4 - Borrow flag
            unsigned _reserved2 : 1;   // 5 - Always 0
            unsigned zf         : 1;   // 6 - Zero flag
            unsigned sf         : 1;   // 7 - Sign flag
            unsigned tf         : 1;   // 8 - Trap flag
            unsigned intf       : 1;   // 9 - Interrupt flag
            unsigned df         : 1;   // 10 - Direction flag
            unsigned of         : 1;   // 11 - Overflow flag
            unsigned iopl       : 2;   // 12 - 13 - I/O privilege level
            unsigned nt         : 1;   // 14 - Nested task flag
            unsigned _reserved3 : 1;   // 15 - Always 0
            unsigned rf         : 1;   // 16 - Resume flag
            unsigned vm         : 1;   // 17 - Virtual 8086 mode
            unsigned ac         : 1;   // 18 - Alignment check
            unsigned vif        : 1;   // 19 - Virtual interrupt flag
            unsigned vip        : 1;   // 20 - Virtual interrupt pending
            unsigned id         : 1;   // 21 - Identification flag
            unsigned _reserved4 : 10;  // 22 - 31 - Always 0
#ifdef _WIN64
            unsigned _zero0     : 32;  // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} FLAGS_REGISTER;


typedef struct _CR0_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned pe         : 1;   // 0 - Protected Mode Enabled
            unsigned mp         : 1;   // 1 - Monitor Coprocessor
            unsigned em         : 1;   // 2 - Emulate
            unsigned ts         : 1;   // 3 - Task Switched
            unsigned et         : 1;   // 4 - Extension Type
            unsigned ne         : 1;   // 5 - Numeric Error
            unsigned _reserved0 : 10;  // 6 - 15
            unsigned wp         : 1;   // 16 - Write Protect
            unsigned _reserved1 : 1;   // 17
            unsigned am         : 1;   // 18 - Alignment Mask
            unsigned _reserved2 : 10;  // 19 - 28
            unsigned nw         : 1;   // 29 - Not Write-Through
            unsigned cd         : 1;   // 30 - Cache Disable
            unsigned pg         : 1;   // 31 - Paging Enabled
#ifdef _WIN64
            unsigned _zero0     : 32;  // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} CR0_REGISTER;


typedef struct _CR3_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            UINT_PTR _reserved0 : 3; // 0 - 2
            UINT_PTR PWT        : 1; // 3
            UINT_PTR PCD        : 1; // 4
            UINT_PTR _reserved1 : 7; // 5 - 11
#ifndef _WIN64
            UINT_PTR PDB        : 20; // 12 - 31
#else
            UINT_PTR PDB        : 52; // 12 - 63
#endif
        } f;
    } u;
} CR3_REGISTER;


typedef struct _CR4_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned vme        : 1;  // 0 - Virtual Mode Extensions
            unsigned pvi        : 1;  // 1 - Protected-Mode Virtual Interrupts
            unsigned tsd        : 1;  // 2 - Time Stamp Disable
            unsigned de         : 1;  // 3 - Debugging Extensions
            unsigned pse        : 1;  // 4 - Page Size Extensions
            unsigned pae        : 1;  // 5 - Physical Address Extension
            unsigned mce        : 1;  // 6 - Machine-Check Enable
            unsigned pge        : 1;  // 7 - Page Global Enable
            unsigned pce        : 1;  // 8 - Performance-Monitoring Counter Enable
            unsigned osfxsr     : 1;  // 9 - OS Support for FXSAVE/FXRSTOR
            unsigned osxmmexcpt : 1;  // 10 - OS Support for Unmasked SIMD Exceptions
            unsigned umip       : 1;  // 11 - User mode instruction prevention
            unsigned _reserved0 : 1;  // 12
            unsigned vmxe       : 1;  // 13 - Virtual Machine Extensions Enabled
            unsigned smxe       : 1;  // 14 - SMX-Enable Bit
            unsigned _reserved1 : 1;  // 15
            unsigned fsgsbase   : 1;  // 16 - FS/GS R/W Instructions enable
            unsigned pcide      : 1;  // 17 - PCID Enable
            unsigned osxsave    : 1;  // 18 - XSAVE and Processor Extended States-Enable
            unsigned _reserved2 : 1;  // 19
            unsigned smep       : 1;  // 20 - Supervisor Mode Execution Protection Enable
            unsigned smap       : 1;  // 21 - Supervisor Mode Access Protection Enable
            unsigned pke        : 1;  // 22 - Protection Key Enable
            unsigned _reserved3 : 9;  // 23 - 31
#ifdef _WIN64
            unsigned _zero0     : 32; // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} CR4_REGISTER;


typedef struct _DR7_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned L0     : 1;  // 0 - Local Breakpoint Enable 0
            unsigned G0     : 1;  // 1 - Global Breakpoint Enable 0
            unsigned L1     : 1;  // 2 - Local Breakpoint Enable 1
            unsigned G1     : 1;  // 3 - Global Breakpoint Enable 1
            unsigned L2     : 1;  // 4 - Local Breakpoint Enable 2
            unsigned G2     : 1;  // 5 - Global Breakpoint Enable 2
            unsigned L3     : 1;  // 6 - Local Breakpoint Enable 3
            unsigned G3     : 1;  // 7 - Global Breakpoint Enable 3
            unsigned LE     : 1;  // 8 - Local Exact Breakpoint Enable
            unsigned GE     : 1;  // 9 - Global Exact Breakpoint Enable
            unsigned _one0  : 1;  // 10 - Always 1
            unsigned RTM    : 1;  // 11 - Restricted Transactional Memory
            unsigned _zero0 : 1;  // 12 - Always zero
            unsigned GD     : 1;  // 13 - General Detect Enable
            unsigned _zero1 : 2;  // 14 - 15 - Always zero
            unsigned RW0    : 2;  // 16 - 17 - Condition BP0
            unsigned LEN0   : 2;  // 18 - 19 - Length BP0
            unsigned RW1    : 2;  // 20 - 21 - Condition BP1
            unsigned LEN1   : 2;  // 22 - 23 - Length BP1
            unsigned RW2    : 2;  // 24 - 25 - Condition BP2
            unsigned LEN2   : 2;  // 26 - 27 - Length BP2
            unsigned RW3    : 2;  // 28 - 29 - Condition BP3
            unsigned LEN3   : 2;  // 30 - 31 - Length BP3
#ifdef _WIN64
            unsigned _zero2 : 32;  // 32 - 63 - Must be zero od #PG
#endif
        } f;
    } u;
} DR7_REGISTER;


typedef struct _DR6_REGISTER
{
    union
    {
        UINT_PTR raw;

        struct
        {
            unsigned B0     : 1;  // 0 - Breakpoint Condition Detected 0
            unsigned B1     : 1;  // 1 - Breakpoint Condition Detected 1
            unsigned B2     : 1;  // 2 - Breakpoint Condition Detected 2
            unsigned B3     : 1;  // 3 - Breakpoint Condition Detected 3
            unsigned _one0  : 8;  // 4 - 11 - Always 1
            unsigned _zero0 : 1;  // 12 - Always 0
            unsigned BD     : 1;  // 13 - DR access detected
            unsigned BS     : 1;  // 14 - Single step
            unsigned BT     : 1;  // 15 - Task switch
            unsigned RTM    : 1;  // 16 - Restricted transactional memory
            unsigned _one1  : 15; // 17 - 31 - Always 1
#ifdef _WIN64
            unsigned _zero1 : 32; // 32 - 63 - Must be zero od #PG
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
            unsigned offsetLow    : 16;
            unsigned selector     : 16;
#ifdef _WIN64
            unsigned ist          : 3;
            unsigned _reserved0   : 5;
#else
            unsigned _reserved0   : 8;
#endif
            unsigned type         : 5;
            unsigned dpl          : 2;
            unsigned present      : 1;
            unsigned offsetHigh   : 16;
        } f;
    } u;
#ifdef _WIN64
    UINT32    offsetUpper;
    UINT32    _reserved0;
#endif
} IDT_ENTRY, *PIDT_ENTRY;


typedef struct _SEGMENT_SELECTOR
{
    union
    {
        UINT16 raw;

        struct 
        {
            unsigned rpl   : 2;
            unsigned ti    : 1;
            unsigned index : 13;
        } f;
    } u;
} SEGMENT_SELECTOR, *PSEGMENT_SELECTOR;


typedef struct _SEGMENT_DESCRIPTOR
{
    union
    {
        UINT64 raw;

        struct
        {
            unsigned limitLow  : 16;
            unsigned baseLow   : 16;
            unsigned baseMid   : 8;
            unsigned type      : 4;
            unsigned system    : 1;
            unsigned dpl       : 2;
            unsigned present   : 1;
            unsigned limitHigh : 4;
            unsigned avl       : 1;
            unsigned l         : 1;  // 64-bit code segment (IA-32e mode only)
            unsigned db        : 1;
            unsigned gran      : 1;
            unsigned baseHigh  : 8;
        } f;
    } u;
#ifdef _WIN64
    UINT32 baseUpper;
    UINT32 _reserved0;
#endif
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;


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
    _In_      UINT_PTR   idt,
    _In_      UINT8      service,
    _In_      UINT_PTR   handler,
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
UINT_PTR
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
    _In_ UINT_PTR   idt,
    _In_ UINT8      service,
    _In_ PIDT_ENTRY old
);

/**
* Segment selector reading.
*/
UINT16 __readcs(VOID);
UINT16 __readss(VOID);
UINT16 __readds(VOID);
UINT16 __reades(VOID);
UINT16 __readfs(VOID);
UINT16 __readgs(VOID);
UINT16 __str   (VOID);
UINT16 __sldt  (VOID);

/**
* Segment selector writing.
* All receive a 16-bit-wide value containing the selector.
*/
VOID __writess(_In_ UINT16 selector);
VOID __writeds(_In_ UINT16 selector);
VOID __writees(_In_ UINT16 selector);
VOID __writefs(_In_ UINT16 selector);
VOID __writegs(_In_ UINT16 selector);
VOID __ltr    (_In_ UINT16 selector);

/**
* Segment descriptor reading.
* Given a selector, it is indexed in the appropiate table (GDT/LDT) and 
* different attributes are read. 
* Those attributes are base, limit, and access rights of the descriptor.
*/
UINT_PTR DescriptorBase        (_In_ UINT16 selector);
UINT32   DescriptorLimit       (_In_ UINT16 selector);
UINT32   DescriptorAccessRights(_In_ UINT16 selector);

/**
* GDT register related functions.
* Reading is done separately (one function to obtain the base and another to get limit).
* Writing is done with a single function (entire register is modified).
*/
UINT_PTR sgdt_base (VOID);
UINT16     sgdt_limit(VOID);
VOID     lgdt      (_In_ UINT_PTR base, _In_ UINT16 limit);

/**
* IDT register related functions.
* Reading is done separately (one function to obtain the base and another to get limit).
* Writing is done with a single function (entire register is modified).
*/
UINT_PTR sidt_base (VOID);
UINT16   sidt_limit(VOID);
VOID     lidt      (_In_ UINT_PTR base, _In_ UINT16 limit);

/**
* Retrieve the value of the stack pointer (esp/rsp).
*/
UINT_PTR __readsp(VOID);

/**
* Retrieve the value of the flags register (eflags/rflags).
*/
UINT_PTR __readflags(VOID);

/**
* Write to cr2 (page-fault address recording register).
*/
VOID __writecr2(_In_ UINT_PTR cr2);


/**
* Read/Write from/to cr/dr registers
*/
UINT_PTR readcr (_In_ UINT32 cr);
VOID     writecr(_In_ UINT32 cr, _In_ UINT_PTR value);

UINT_PTR readdr (_In_ UINT32 dr);
VOID     writedr(_In_ UINT32 dr, _In_ UINT_PTR value);

/**
* cli and sti instructions instrinsics.
* Inhibit and habilitate interrupts in the executing core.
*/
VOID __cli(VOID);
VOID __sti(VOID);

/**
* Pause instruction instrinsic.
* Used in synchonization bucles.
*/
VOID __pause(VOID);

#endif
