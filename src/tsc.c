#include "dsdef.h"
#include "wpp.h"
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

#define IS_SIBLING(Sibling) (Sibling->After.Address != 0) && (Sibling->Before.Address != 0)

//
// Get first free slot available, or return oldest hit instead.
//
PTSC_ENTRY GetSiblingSlot(
    _In_ PTSC_ENTRY Head
) 
{
    PTSC_ENTRY Sibling = NULL;
    PTSC_ENTRY OrphanOldest = NULL;
    PTSC_ENTRY SiblingOldest = NULL;
    UINT32     i;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
         Sibling = &Head[i];

        if ( IsFreeSlot(Sibling) ) {
            return Sibling;
        } else {
            //
            // Initialize with the first non-free slot available
            //
            if ( IS_SIBLING(Sibling) ) {
                if ( SiblingOldest == NULL ) { SiblingOldest = Sibling; }

                if ( Sibling->Before.TimeStamp < SiblingOldest->Before.TimeStamp ) {
                    SiblingOldest = Sibling;
                }

            } else {
                if ( OrphanOldest == NULL ) { OrphanOldest = Sibling; }

                if ( Sibling->Before.TimeStamp < OrphanOldest->Before.TimeStamp ) {
                    OrphanOldest = Sibling;
                }
            }

        }
    }

    NT_ASSERT( SiblingOldest != NULL || OrphanOldest != NULL );

    //
    // If we went to here, means that we are recycling an entry
    // let's clear addresses. Also, there are priority for Siblings
    // to remain alive.
    //
    if ( OrphanOldest ) {
        ClearSibling(OrphanOldest);
        return OrphanOldest;
    }
    else {
        ClearSibling(SiblingOldest);
        return SiblingOldest;
    }
}

#define ADDRESS_CLEAR_LAST_BYTE(address) (address & 0xFFFFFFFFFFFFFF00)
#define ADDRESSES_ARE_BYTE_SHORT(source, target) abs(source - target) < 0xFF

//
// This function requires to reorder the lists 
// so lookup will be optimized
//
PTSC_ENTRY FindSibling(
    _In_ PTSC_ENTRY Head, 
    _In_ UINTN   Process,
    _In_ ULONG_PTR  OffensiveAddress
) 
{
    PTSC_ENTRY Entry = NULL;
    INT        i;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
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
        //  Consider also, instructions that potentially are between page boundaries.
        //
        //              0000789FFFFFFFFF | 0F 31 | rdtsc | ; Before
        //              000078A000000001 | 0F A2 | cpuid |
        //              000078A000000003 | 0F 31 | rdtsc | ; After
        //
        if ( Process == Entry->Process &&
             ADDRESSES_ARE_BYTE_SHORT(Entry->Before.Address, OffensiveAddress) ) {

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

        NT_ASSERT( ADDRESSES_ARE_BYTE_SHORT(Sibling->Before.Address, OffensiveAddress) );

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
            UINT64 DifferenceThreshold = ( Sibling->Difference * 3 / 2 );

            if (( Difference > Sibling->Difference ) &&
                ( CurrentDifference > DifferenceThreshold )) {
                //
                // If the average isn't below threshold let's skip it
                //
                Sibling->Skips += 1;
            } else {
                //
                // Let's keep a consistent average
                //
                Sibling->Difference += Difference;
                Sibling->Difference /= 2;
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
    if ( Sibling->Difference > 0x50000 ) {
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
    _In_ UINTN       Process,
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

BOOLEAN
ProcessTscEvent(
    _In_ PTSC_ENTRY Head,
    _In_ ULONG_PTR OffensiveAddress,
    _In_ UINTN Process,
    _In_ ULARGE_INTEGER TimeStamp
    )
{
    DS_NOTIFICATION_MESSAGE Notification;

    PTSC_ENTRY Sibling = FindSibling( Head, Process, OffensiveAddress );

    if ( Sibling ) {
        SiblingIncrement( Sibling, OffensiveAddress, TimeStamp );

    } else {
        Sibling = CreateSibling( Head, OffensiveAddress, Process, TimeStamp );
    }

    // 
    //  This will be the trigger of a detection.
    //
    if (IsTimmingAttack( Sibling )) {
        RtlPostMailboxTrace( &gSecureMailbox,
                             TRACE_LEVEL_INFORMATION,
                             TRACE_IOA_ROOT,
                             "Timing Attack Found (ProcessId = %p)\n",
                             PsGetCurrentProcessId() );

        Notification.ControlFlags = 0;
        Notification.MessageType = NotificationMessage;

        Notification.ProcessId = (UINT64)PsGetCurrentProcessId();
        Notification.ThreadId = (UINT64)PsGetCurrentThreadId();
        Notification.Type = TimerAbuse;

        RtlPostMailboxNotification( &gSecureMailbox, 
                                    &Notification,
                                    sizeof( DS_NOTIFICATION_MESSAGE ) );

        return TRUE;
    }

    return FALSE;
}

VOID
InjectTerminateProcess(
    _Inout_ PUINT8 Mapping,
    _In_ PGP_REGISTERS Registers
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
                                   0xB8, 0x29, 0x00, 0x00, 0x00,
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

    RtlCopyMemory( Mapping, TerminateProcessStub, sizeof( TerminateProcessStub ) );

    //
    //  Rip will point to TerminateProcessStub.
    //
    Registers->Rip = (UINTN) PAGE_ALIGN(Registers->Rip);
}

VOID
RdtscEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS     Registers,
    _In_ UINTN       Process,
    _In_ PUINT8         Mapping
)
{
    ULARGE_INTEGER TimeStamp = { 0 };
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Registers->Rdx = TimeStamp.HighPart;
    Registers->Rax = TimeStamp.LowPart;

    if (ProcessTscEvent(Local->TscHits, Registers->Rip, Process, TimeStamp) ) {
        InjectTerminateProcess(Mapping, Registers);
        return;
    }

    InstrRipAdvance(Registers);
}

VOID
RdtscpEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS     Registers,
    _In_ UINTN       Process,
    _In_ PUINT8         Mapping
)
{
    LARGE_INTEGER Processor = { 0 };
    ULARGE_INTEGER TimeStamp = { 0 };

    Processor.QuadPart = __readmsr(IA32_TSC_AUX);

    Registers->Rcx = Processor.LowPart;

    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Registers->Rdx = TimeStamp.HighPart;
    Registers->Rax = TimeStamp.LowPart;
    Registers->Rcx = Processor.LowPart;

    if (ProcessTscEvent(Local->TscHits, Registers->Rip, Process, TimeStamp) ) {
        InjectTerminateProcess(Mapping, Registers);
        return;
    }

    InstrRipAdvance(Registers);
}

VOID
EnableUserTimeStamp(
    VOID
    )
{
    __writecr4( __readcr4() & ~CR4_TSD );
}

VOID
DisableUserTimeStamp(
    VOID
    )
{
    __writecr4( __readcr4() | CR4_TSD );
}
