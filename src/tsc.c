#include "wdk7.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "x86.h"

BOOLEAN
IsFreeSlot(
    _In_ PTSC_ENTRY Entry
)
{
    return Entry->Before.Address == 0;
}

//
// Get first free slot available, or return oldest hit instead.
//
PTSC_ENTRY GetSiblingSlot(
    _In_ PTSC_ENTRY Head
) 
{
    PTSC_ENTRY Sibling = NULL;
    PTSC_ENTRY Oldest = NULL;

    for (UINT32 i = 0; i < MAX_TSC_HITS; i++) {
         Sibling = &Head[i];

        if ( IsFreeSlot(Sibling) ) {
            return Sibling;
        } else {
            //
            // Initialize with the first non-free slot available
            //
            if ( Oldest == NULL ) { Oldest = Sibling; }

            if ( Oldest->Before.TimeStamp > Sibling->Before.TimeStamp ) {
                Oldest = Sibling;
            }
        }
    }

    NT_ASSERT( Oldest != NULL );

    //
    // If we went to here, means that we are recycling an entry
    // let's clear addresses
    //
    Oldest->Before.Address = 0;
    Oldest->After.Address = 0;

    return Oldest;
}

#define ADDRESS_CLEAR_LAST_BYTE(address) (address & 0xFFFFFFFFFFFFFF00)

//
// This function requires to reorder the lists 
// so lookup will be optimized
//
PTSC_ENTRY FindSibling(
    _In_ PTSC_ENTRY Head, 
    _In_ ULONG_PTR  OffensiveAddress
) 
{
    PTSC_ENTRY Entry = NULL;

    for ( int i = 0; i < MAX_TSC_HITS; i++ ) {
        Entry = &Head[i];
        
        // 
        // In order to be considered as sibling, it must match this sentence:
        // 
        // Address is equal except last byte, example: 
        //
        //              00007FF6AED493E0 | 0F 31 | rdtsc | ; Before
        //              00007FF6AED493E2 | 0F A2 | cpuid |
        //              00007FF6AED493E4 | 0F 31 | rdtsc | ; After
        //
        // Also, at least one of the addresses should be recorded as "Before" or "After"
        // if not, we consider it as a potential "After" so current Sibling should have
        // After cleared.
        //
        if ( ADDRESS_CLEAR_LAST_BYTE(Entry->Before.Address) == ADDRESS_CLEAR_LAST_BYTE(OffensiveAddress) ) {

            if (( Entry->Before.Address == OffensiveAddress ) ||
                ( Entry->After.Address  == OffensiveAddress ) ||
                ( Entry->After.Address  == 0 )) {
                return Entry;
            } else { continue; }
        }
    }

    return NULL;
}

VOID HitIncrement(
    _In_ PTSC_HIT       Hit,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    //
    // There is no need for atomic operations since there are 
    // no interruptions here.
    //
    Hit->Count    += 1;
    Hit->TimeStamp = TimeStamp.QuadPart;
}

VOID SiblingIncrement(
    _In_ PTSC_ENTRY     Sibling,
    _In_ ULONG_PTR      OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    PTSC_HIT Hit = NULL;

    if ( Sibling->Before.Address == OffensiveAddress ) {
        Hit = &Sibling->Before;
    } else if ( Sibling->After.Address == OffensiveAddress ){
        Hit = &Sibling->After;
    } else {
        //
        // This means that we reached a potential "After" candidate
        //
        NT_ASSERT( ADDRESS_CLEAR_LAST_BYTE(Sibling->Before.Address) == 
                   ADDRESS_CLEAR_LAST_BYTE(OffensiveAddress) );

        if ( Sibling->After.Address == 0 ) {

            //
            // Assign detected address
            //
            Sibling->After.Address = OffensiveAddress;
            Hit = &Sibling->After;
        }
        else {
            //
            // If we reach this assert it means that we could have potentially this situation
            //
            //              00007FF6AED493E0 | 0F 31 | rdtsc | ; Before
            //              00007FF6AED493E2 | 0F A2 | cpuid |
            //              00007FF6AED493E4 | 0F 31 | rdtsc | ; After
            //              00007FF6AED493E6 | 0F 31 | rdtsc | ; Not recognized
            //
            // where at somepoint, a third candidate is evaluated as sibling without 
            // any reference.
            // 
            NT_ASSERT(FALSE);
        }
    }

    NT_ASSERT(Hit != NULL);

    HitIncrement(Hit, TimeStamp);
}

//
// First dummy detection of a timming attack
//
BOOLEAN IsTimmingAttack(
    _In_ PTSC_ENTRY     Sibling
)
{
    return (((Sibling->After.TimeStamp - Sibling->Before.TimeStamp) < 500000) &&
             (Sibling->Before.Count > 255) && (Sibling->After.Count > 255));
}

PTSC_ENTRY CreateSibling(
    _In_ PTSC_ENTRY     Head,
    _In_ ULONG_PTR      OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    PTSC_ENTRY Sibling = GetSiblingSlot(Head);

    Sibling->Before.Address   = OffensiveAddress;
    Sibling->Before.Count     = 1;
    Sibling->Before.TimeStamp = TimeStamp.QuadPart;

    return Sibling;
}

VOID ProcessTscEvent(
    _In_ PTSC_ENTRY     Head,
    _In_ ULONG_PTR      OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    PTSC_ENTRY Sibling = FindSibling(Head, OffensiveAddress);

    if ( Sibling ) {
        SiblingIncrement(Sibling, OffensiveAddress, TimeStamp);
    } else {
        Sibling = CreateSibling(Head, OffensiveAddress, TimeStamp);
    }

    // 
    // This will be the trigger of a detection
    //
    // IsTimmingAttack(Sibling);
}

VOID
RdtscEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS Regs
)
{
    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;

    ProcessTscEvent(Local->TscHits, Regs->rip, TimeStamp);

    InstrRipAdvance(Regs);
}

VOID
RdtscpEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS Regs
)
{
    UNREFERENCED_PARAMETER(Local);

	UINT64 Processor = __readmsr(IA32_TSC_AUX);

    Regs->rcx = Processor;

    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;
    Regs->rcx = Processor;

    ProcessTscEvent(Local->TscHits, Regs->rip, TimeStamp);

    InstrRipAdvance(Regs);
}

VOID
EnableUserTimeStamp(
    VOID
    )
{
    CR4_REGISTER cr4 = { 0 };
    cr4.u.raw = __readcr4();

    // disable TimeStamp when requested from user-mode
    cr4.u.f.tsd = 1;

    __writecr4(cr4.u.raw);
}

VOID
DisableUserTimeStamp(
    VOID
    )
{
    CR4_REGISTER cr4 = { 0 };

    cr4.u.raw = __readcr4();

    // disable TimeStamp when requested from user-mode
    cr4.u.f.tsd = 0;

    __writecr4(cr4.u.raw);
}
