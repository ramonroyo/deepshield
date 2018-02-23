#include "wdk7.h"
#include "x86.h"

VOID 
IdtInstallService(
    _In_      UINT_PTR   idt,
    _In_      UINT8      service,
    _In_      UINT_PTR   handler,
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
    tmp.u.f.selector   = __readcs();
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

    //
    // Copy
    //
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

UINT_PTR
IdtReadServiceAddressFromEntry(
    _In_ PIDT_ENTRY entry
)
{
    UINT_PTR address;

    address = (UINT32)((entry->u.f.offsetHigh << 16) | entry->u.f.offsetLow);

#ifdef _WIN64
    address |= (((UINT64)(entry->offsetUpper)) << 32);
#endif

    return address;
}

VOID 
IdtRestoreEntry(
    _In_ UINT_PTR   idt,
    _In_ UINT8      service,
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


UINT_PTR __stdcall
DescriptorBase(
    _In_ UINT16 selector
)
{
    SEGMENT_SELECTOR    segment;
    PSEGMENT_DESCRIPTOR descriptor;
    UINT_PTR            base;

    //
    // descriptor = GDT[_selector]
    //
    segment.u.raw = selector;

    descriptor = (PSEGMENT_DESCRIPTOR)((PUINT64)sgdt_base() + segment.u.f.index);

    //
    // base[24:31] = descriptor[56:63]
    // base[16:23] = descriptor[31:40]
    // base[0 :15] = descriptor[16:31]
    //
    base = (UINT_PTR)(descriptor->u.f.baseLow | (descriptor->u.f.baseMid << 16) | (descriptor->u.f.baseHigh << 24));

#ifdef _WIN64
    //
    // If bit S(44) == 0 ==> SYSTEM descriptor... in x64 only system descriptors are expanded to 64 bits bases
    //
    if (descriptor->u.f.system == 0)
    {
        //
        // base[32:63] = highDescriptor
        //
        base |= (((UINT64)descriptor->baseUpper) << 32);
    }
#endif

    return base;
}


UINT32 __stdcall
DescriptorAccessRights(
    _In_ UINT16 selector
)
{
    SEGMENT_SELECTOR    segment;
    PSEGMENT_DESCRIPTOR descriptor;
    UINT32              accessRights;

    //
    // descriptor = GDT[_selector]
    //
    segment.u.raw = selector;

    descriptor = (PSEGMENT_DESCRIPTOR)((PUINT64)sgdt_base() + segment.u.f.index);

    //
    // accessRights[0:7]   = descriptor[40:47]
    // accessRights[8:11]  = 0
    // accessRights[12:15] = descriptor[52:55]
    //
    accessRights = (UINT32)((descriptor->u.raw & 0x00F0FF0000000000) >> 40);

    //
    // If null selector is used, accessRights are marked as invalid
    //
    if (!selector)
        accessRights |= 0x10000;

    return accessRights;
}

extern VOID __stdcall __sgdt(PVOID memory);
extern VOID __stdcall __lgdt(PVOID memory);

UINT_PTR
sgdt_base(
    VOID
)
{
    UINT8 gdt[sizeof(UINT16) + sizeof(UINT_PTR)];

    __sgdt(&gdt);

    return *(PUINT_PTR)((PUINT8)&gdt + 2);
}


UINT16
sgdt_limit(
    VOID
)
{
    UINT8 gdt[sizeof(UINT16) + sizeof(UINT_PTR)];

    __sgdt(&gdt);

    return *(PUINT16)&gdt;
}


VOID
lgdt(
    _In_ UINT_PTR base,
    _In_ UINT16   limit
)
{
    UINT8 gdt[sizeof(UINT16) + sizeof(UINT_PTR)];

    *(PUINT16)  ((PUINT8)&gdt + 0) = limit;
    *(PUINT_PTR)((PUINT8)&gdt + 2) = base;
    
    __lgdt(&gdt);
}


UINT_PTR
sidt_base(
    VOID
)
{
    UINT8 idt[sizeof(UINT16) + sizeof(UINT_PTR)];

    __sidt(&idt);

    return *(PUINT_PTR)((PUINT8)&idt + 2);
}

UINT16
sidt_limit(
    VOID
)
{
    UINT8 idt[sizeof(UINT16) + sizeof(UINT_PTR)];

    __sidt(&idt);

    return *(PUINT16)&idt;
}


VOID
lidt(
    _In_ UINT_PTR base,
    _In_ UINT16   limit
)
{
    UINT8 idt[sizeof(UINT16) + sizeof(UINT_PTR)];

    *(PUINT16)  ((PUINT8)&idt + 0) = limit;
    *(PUINT_PTR)((PUINT8)&idt + 2) = base;

    __lidt(&idt);
}


UINT_PTR
readcr(
    _In_ UINT32 cr
)
{
    UINT_PTR value;

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


VOID
writecr(
    _In_ UINT32   cr,
    _In_ UINT_PTR value
)
{
    switch (cr)
    {
        case 0: __writecr0(value); break;
        case 2: __writecr2(value); break;
        case 3: __writecr3(value); break;
        case 4: __writecr4(value); break;
#ifdef _WIN64
        case 8: __writecr8(value); break;
#endif
    }
}


UINT_PTR
readdr(
    _In_ UINT32 dr
)
{
    UINT_PTR value;

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
    _In_ UINT_PTR value
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
