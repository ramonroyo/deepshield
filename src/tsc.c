#include "wdk7.h"
#include "tsc.h"
#include "context.h"
#include "instr.h"
#include "vmx.h"
#include "mmu.h"
#include "smp.h"
#include "mem.h"
#include "x86.h"

VOID
ClearSibling(
    _In_ PTSC_ENTRY Entry
)
{
    memset(Entry, 0, sizeof(TSC_ENTRY));
}

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

            if ( Oldest->Before.TimeStamp < Sibling->Before.TimeStamp ) {
                Oldest = Sibling;
            }
        }
    }

    NT_ASSERT( Oldest != NULL );

    //
    // If we went to here, means that we are recycling an entry
    // let's clear addresses
    //
    ClearSibling(Oldest);

    return Oldest;
}


#define ADDRESS_CLEAR_LAST_BYTE(address) (address & 0xFFFFFFFFFFFFFF00)

//
// This function requires to reorder the lists 
// so lookup will be optimized
//
PTSC_ENTRY FindSibling(
    _In_ PTSC_ENTRY Head, 
    _In_ UINT_PTR   Process,
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
        if ( Process == Entry->Process &&
             ADDRESS_CLEAR_LAST_BYTE(Entry->Before.Address) == ADDRESS_CLEAR_LAST_BYTE(OffensiveAddress) ) {

            if (( Entry->Before.Address == OffensiveAddress ) ||
                ( Entry->After.Address  == OffensiveAddress ) ||
                ( Entry->After.Address  == 0 )) {
                return Entry;
            } 
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
            Sibling->After.Address   = OffensiveAddress;
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

    if ( Hit->Address == Sibling->After.Address ) {
        //
        // We got his brother, let's update difference average
        // 
        UINT64 Difference = abs(TimeStamp.QuadPart - Sibling->Before.TimeStamp);

        if ( Sibling->Difference == 0 ) {
            Sibling->Difference = Difference;
        } else {

            UINT64 CurrentDifference   = abs(Difference - Sibling->Difference);
            UINT64 DifferenceThreshold = ( Difference * 3 / 2 );

            if ( CurrentDifference < DifferenceThreshold ) {
                //
                // Let's keep a consistent average
                //
                Sibling->Difference += Difference;
                Sibling->Difference /= 2;
            } else {
                //
                // If the average isn't below threshold let's skip it
                //
                Sibling->Skips += 1;
            }
        }

    }

    HitIncrement(Hit, TimeStamp);
}

//
// First dummy detection of a timming attack
//
BOOLEAN IsTimmingAttack(
    _In_ PTSC_ENTRY     Sibling
)
{
    BOOLEAN IsSibling          = FALSE;
    BOOLEAN IsSyncThresholdOk  = FALSE; 
    BOOLEAN IsSkipsThresholdOk = FALSE; 
    BOOLEAN IsCountThresholdOk = FALSE; 
    BOOLEAN IsTimmingAttack    = FALSE; 

    if ( Sibling->Difference == 0 ) { return FALSE; }

    IsSibling = Sibling->Before.Address && Sibling->After.Address;

    //
    // Fast filter for empty entries
    //
    if ( !IsSibling ) {
        return FALSE;
    }

    //
    // Any average up to this should be discarded
    //
    if ( !Sibling->Difference > 0x50000 ) {
        ClearSibling(Sibling);
        return FALSE;
    }

    //
    // Count threshold ensures that we, at least, went over 255 retries
    //
    IsCountThresholdOk = Sibling->Before.Count > 255;

    //
    // Difference threshold synchronizes the number of sibling addresses we fetched
    //
    IsSyncThresholdOk  = abs(Sibling->Before.Count - Sibling->After.Count) < 2;

    //
    // Skips threshold ensures that we didn't had more than 5% of fetches flushed
    //
    IsSkipsThresholdOk = Sibling->Skips < max((Sibling->Before.Count / 20), 1);

    IsTimmingAttack = ( IsSyncThresholdOk &&
             IsSkipsThresholdOk &&
             IsCountThresholdOk );

    //
    // If it is not already a timming attack, then clear the Sibling
    //
    if ( !IsTimmingAttack && IsCountThresholdOk ) {
        ClearSibling(Sibling);
    }

    return IsTimmingAttack;
}

PTSC_ENTRY CreateSibling(
    _In_ PTSC_ENTRY     Head,
    _In_ ULONG_PTR      OffensiveAddress,
    _In_ UINT_PTR       Process,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    PTSC_ENTRY Sibling = GetSiblingSlot(Head);

    Sibling->Process          = Process;
    Sibling->Before.Address   = OffensiveAddress;
    Sibling->Before.Count     = 1;
    Sibling->Before.TimeStamp = TimeStamp.QuadPart;

    return Sibling;
}

BOOLEAN ProcessTscEvent(
    _In_ PTSC_ENTRY     Head,
    _In_ ULONG_PTR      OffensiveAddress,
    _In_ UINT_PTR       Process,
    _In_ ULARGE_INTEGER TimeStamp
)
{
    PTSC_ENTRY Sibling = FindSibling(Head, Process, OffensiveAddress);

    if ( Sibling ) {
        SiblingIncrement(Sibling, OffensiveAddress, TimeStamp);
    } else {
        Sibling = CreateSibling(Head, OffensiveAddress, Process, TimeStamp);
    }

    // 
    // This will be the trigger of a detection
    //
    if ( IsTimmingAttack(Sibling) ) {

        return TRUE;
    }

    return FALSE;
}

VOID InjectTerminateProcess(
    PUINT8     Mapping,
    PREGISTERS Regs
)
{

#ifndef _WIN64
    /*
    *   TerminateProcess for 32-bit only
    *   ================================================
    *
    *   push -1                   ; CurrentProcess
    *   mov eax,2C                ; NtTerminateProcess
    *   syscall                
    */
    CHAR TerminateProcessStub[] = {0x48, 0xC7, 0xC1, 0xFF, 0xFF,
                                   0xFF, 0xFF, 0x4C, 0x8B, 0xD1,
                                   0xB8, 0x2C, 0x00, 0x00, 0x00,
                                   0x0F, 0x05};
#else
    /*
    *   TerminateProcess for 64-bit only
    *   ================================================
    *
    *   mov rcx,FFFFFFFFFFFFFFFF  ; CurrentProcess (-1)
    *   mov r10,rcx              
    *   mov eax,2C                ; NtTerminateProcess
    *   syscall                
    */
    CHAR TerminateProcessStub[] = {0x48, 0xC7, 0xC1, 0xFF, 0xFF,
                                   0xFF, 0xFF, 0x4C, 0x8B, 0xD1,
                                   0xB8, 0x2C, 0x00, 0x00, 0x00,
                                   0x0F, 0x05};
#endif

    memcpy(Mapping, TerminateProcessStub, sizeof(TerminateProcessStub));

    // Rip will point to TerminateProcessStub
    Regs->rip = (UINT_PTR) PAGE_ALIGN(Regs->rip);
}

VOID
RdtscEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS     Regs,
    _In_ UINT_PTR       Process,
    _In_ PUINT8         Mapping
)
{
    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Regs->rdx = TimeStamp.HighPart;
    Regs->rax = TimeStamp.LowPart;

    if ( ProcessTscEvent(Local->TscHits, Regs->rip, Process, TimeStamp) ) {
        InjectTerminateProcess(Mapping, Regs);
        return;
    }

    InstrRipAdvance(Regs);
}

VOID
RdtscpEmulate(
    _In_ PLOCAL_CONTEXT Local,
	_In_ PREGISTERS     Regs,
    _In_ UINT_PTR       Process,
    _In_ PUINT8         Mapping
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

    if ( ProcessTscEvent(Local->TscHits, Regs->rip, Process, TimeStamp) ) {
        InjectTerminateProcess(Mapping, Regs);
        return;
    }


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
