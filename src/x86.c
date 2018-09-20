#include <ntifs.h>
#include "wdk7.h"
#include "x86.h"

VOID 
IdtInstallService(
    _In_      UINTN   idt,
    _In_      UINT8      service,
    _In_      UINTN   handler,
    _Out_opt_ PIDT_ENTRY old
)
{
    PIDT_ENTRY entry;
    IDT_ENTRY  tmp;
    
    //
    // Read installed handler
    //
    entry = ((PIDT_ENTRY)idt) + service;

    //
    // Copy to previous if desired
    //
    if (old)
    {
        memcpy(old, entry, sizeof(IDT_ENTRY));
    }

    //
    // Prepare new handler
    //
    tmp.u.raw          = 0;
    tmp.u.f.offsetLow  = ((UINT32)handler) & 0x0000FFFF;
    tmp.u.f.selector   = AsmReadCs();
#ifdef _WIN64
    //tmp.u.f.ist      = entry->u.f.ist;
    tmp.u.f.ist        = 0;
#endif
    tmp.u.f.type       = 0x0E;
    tmp.u.f.dpl        = 0;
    tmp.u.f.present    = 1;
    tmp.u.f.offsetHigh = (((UINT32)handler) & 0xFFFF0000) >> 16;

#ifdef _WIN64
    tmp.offsetUpper = (UINT32)(handler >> 32);
#endif

    memcpy(entry, &tmp, sizeof(IDT_ENTRY));
}

PIDT_ENTRY
IdtGetEntryFromServiceNumber(
    _In_ PVOID  idt,
    _In_ UINT32 service
)
{
    PIDT_ENTRY table;

    table = (PIDT_ENTRY)idt;
    return table + service;
}

UINTN
IdtReadServiceAddressFromEntry(
    _In_ PIDT_ENTRY entry
)
{
    UINTN address;

    address = (UINT32)((entry->u.f.offsetHigh << 16) | entry->u.f.offsetLow);

#ifdef _WIN64
    address |= (((UINT64)(entry->offsetUpper)) << 32);
#endif

    return address;
}

VOID 
IdtRestoreEntry(
    _In_ UINTN idt,
    _In_ UINT8 service,
    _In_ PIDT_ENTRY old
)
{
    PIDT_ENTRY entry;

    //
    // Get entry
    //
    entry = ((PIDT_ENTRY)idt) + service;

    //
    // Copy
    //
    memcpy(entry, old, sizeof(IDT_ENTRY));
}

UINTN
BaseFromSelector(
    _In_ UINT16 Selector
     )
{
    IA32_SEGMENT_SELECTOR Segment;
    PIA32_SEGMENT_DESCRIPTOR Descriptor;
    UINTN Base = 0;

    Segment.AsUint16 = Selector;
    NT_ASSERT( Segment.Bits.Ti == 0 );
    Descriptor = (PIA32_SEGMENT_DESCRIPTOR) 
                        ((PUINT64)GetGdtrBase() + Segment.Bits.Index);

    //
    //  Base[24:31] = Descriptor[56:63]
    //  Base[16:23] = Descriptor[31:40]
    //  Base[0 :15] = Descriptor[16:31]
    //

    Base = (UINT32) (Descriptor->BaseLow
                  | (Descriptor->Bytes.BaseMiddle << 16)
                  | (Descriptor->Bytes.BaseHigh << 24));

#ifdef _WIN64
    //
    //  TSS descriptor (TR selector) and LDR descriptor has 16 
    //  bytes in 64-bit mode. Check System bit for TSS, LDT and
    //  gate descriptors, otherwise this is a code, stack or
    //  data segment descriptor.
    //
    Base |= (Descriptor->Bits.System == 0) ?
            ((UINT64)Descriptor->BaseUpper << 32) : 0;
#endif

    return Base;
}

UINT32
ArFromSelector(
    _In_ UINT16 Selector
    )
{
    IA32_SEGMENT_SELECTOR Segment;
    PIA32_SEGMENT_DESCRIPTOR Descriptor;
    UINT32 AccessRights;

    Segment.AsUint16 = Selector;
    Descriptor = (PIA32_SEGMENT_DESCRIPTOR)
                        ((PUINT64)GetGdtrBase() + Segment.Bits.Index);

    //
    //  AccessRights[0 : 7] = Descriptor[40:47]
    //  AccessRights[8 :11] = 0
    //  AccessRights[12:15] = Descriptor[52:55]
    //

    AccessRights = (UINT32)((Descriptor->DataLow & 0x00F0FF0000000000) >> 40);

    if (Selector == 0) {
        AccessRights |= 0x10000;
    }

    return AccessRights;
}

UINTN
GetGdtrBase(
    VOID
    )
{
    IA32_DESCRIPTOR Gdt;

    AsmReadGdtr( &Gdt );
    return Gdt.Base;
}

UINT16
GetGdtrLimit(
    VOID
    )
{
    IA32_DESCRIPTOR Gdt;

    AsmReadGdtr( &Gdt );
    return Gdt.Limit;
}

UINTN
GetIdtrBase(
    VOID
    )
{
    IA32_DESCRIPTOR Idt;

    __sidt( &Idt );
    return Idt.Base;
}

UINT16
GetIdtrLimit(
    VOID
    )
{
    IA32_DESCRIPTOR Idt;

    __sidt( &Idt );
    return Idt.Limit;
}

VOID
WriteIdtr(
    _In_ IA32_DESCRIPTOR Idt
    )
{
    __lidt( &Idt );
}

UINTN
readcr(
    _In_ UINT32 cr
    )
{
    UINTN value;

    value = 0;

    switch (cr)
    {
        case 0: value = __readcr0(); break;
        case 2: value = __readcr2(); break;
        case 3: value = __readcr3(); break;
        case 4: value = __readcr4(); break;
#ifdef _WIN64
        case 8: value = __readcr8(); break;
#endif
    }

    return value;
}

_IRQL_raises_(value)
VOID
writecr(
    _In_ UINT32 cr,
    _In_ UINTN value
    )
{
    switch (cr)
    {
        case 0: __writecr0(value); break;
#ifdef _WIN64
        case 2: __writecr2(value); break;
#endif
        case 3: __writecr3(value); break;
        case 4: __writecr4(value); break;
#ifdef _WIN64
        case 8: __writecr8(value); break;
#endif
    }
}


UINTN
readdr(
    _In_ UINT32 dr
    )
{
    UINTN value;

    value = 0;

    switch (dr)
    {
        case 0: value = __readdr(0); break;
        case 1: value = __readdr(1); break;
        case 2: value = __readdr(2); break;
        case 3: value = __readdr(3); break;
        case 6: value = __readdr(6); break;
        case 7: value = __readdr(7); break;
    }

    return value;
}

VOID
writedr(
    _In_ UINT32   dr,
    _In_ UINTN value
    )
{
    switch (dr)
    {
        case 0: __writedr(0, value); break;
        case 1: __writedr(1, value); break;
        case 2: __writedr(2, value); break;
        case 3: __writedr(3, value); break;
        case 6: __writedr(6, value); break;
        case 7: __writedr(7, value); break;
    }
}