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
#include "udis86.h"


#define SIBLING_DISTANCE_LIMIT     0xFF
#define IS_ORPHAN_SIBLING( Sibling )    \
    (((Sibling)->After.Address == 0) || ((Sibling)->Before.Address == 0))


#define MASK_VA_LSB(address) (address & 0xFFFFFFFFFFFFFF00)
#define ARE_VAS_WITHIN_SIBLING_RANGE(source, target) \
    (abs((int)(source - target)) < SIBLING_DISTANCE_LIMIT)

NTSTATUS
TdMessageNotify(
    _In_ DS_NOTIFICATION_TYPE Type,
    _In_ UINTN ProcessId,
    _In_ UINTN ThreadId
    )
{
    DS_NOTIFICATION_MESSAGE Notification;

    Notification.ControlFlags = 0;
    Notification.MessageType = NotificationMessage;

    Notification.ProcessId = ProcessId;
    Notification.ThreadId = ThreadId;
    Notification.Type = Type;

    return RtlPostMailboxNotification( &gSecureMailbox, 
                                       &Notification,
                                       sizeof( DS_NOTIFICATION_MESSAGE ) );
}

VOID
TdTimmingFalsePositiveNotify(
    _In_ UINTN ProcessId,
    _In_ UINTN ThreadId
    )
{
    NTSTATUS Status;

    Status = TdMessageNotify( TimerFalsePositive, ProcessId, ThreadId );

    if (!NT_SUCCESS( Status ) ) {
        RtlPostMailboxTrace( &gSecureMailbox,
                             TRACE_LEVEL_WARNING,
                             TRACE_IOA_ROOT,
                             "Unable to notify timming attack (ProcessId = %p, ThreadId = %p)\n",
                             ProcessId, 
                             ThreadId );
    }
}

VOID
TdTimmingAttackNotify(
    _In_ UINTN ProcessId, 
    _In_ UINTN ThreadId
) 
{
    NTSTATUS Status;

    Status = TdMessageNotify( TimerAbuse, ProcessId, ThreadId );

    if (!NT_SUCCESS( Status )) {
        RtlPostMailboxTrace( &gSecureMailbox,
                             TRACE_LEVEL_WARNING,
                             TRACE_IOA_ROOT,
                             "Unable to notify timming attack (ProcessId = %p, ThreadId = %p)\n",
                             ProcessId, 
                             ThreadId );
    }
}

VOID
TdClearSibling(
    _Out_ PTSC_ENTRY Entry
    )
{
    RtlZeroMemory( Entry, sizeof( TSC_ENTRY ) );
}

BOOLEAN
TdIsFreeSlot(
    _In_ PTSC_ENTRY Entry
    )
{
    return Entry->Before.Address == 0;
}

//
// Get first free slot available, or return oldest hit instead.
//
PTSC_ENTRY
TdGetSiblingSlot(
    _In_ PTSC_ENTRY Head
    )
{
    PTSC_ENTRY Sibling = NULL;
    PTSC_ENTRY OrphanOldest = NULL;
    PTSC_ENTRY SiblingOldest = NULL;
    UINT32     i;

    for ( i = 0; i < MAX_TSC_HITS; i++ ) {
         Sibling = &Head[i];

        if (TdIsFreeSlot( Sibling) ) {
            return Sibling;

        } else {
            //
            // Initialize with the first non-free slot available
            //
            if (!IS_ORPHAN_SIBLING( Sibling ) ) {
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
        TdClearSibling( OrphanOldest );
        return OrphanOldest;
    }
    else {
        TdClearSibling( SiblingOldest );
        return SiblingOldest;
    }
}

//
// This function requires to reorder the lists 
// so lookup will be optimized
//
PTSC_ENTRY TdFindSibling(
    _In_ PTSC_ENTRY Head, 
    _In_ UINTN   TscHash,
    _In_ UINTN  OffensiveAddress
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
        if ( TscHash == Entry->TscHash &&
             ARE_VAS_WITHIN_SIBLING_RANGE(Entry->Before.Address, OffensiveAddress) ) {

            if (( Entry->Before.Address == OffensiveAddress ) ||
                ( Entry->After.Address  == OffensiveAddress ) ||
                ( Entry->After.Address  == 0 )) {
                return Entry;
            } 
        }
    }

    return NULL;
}

VOID TdHitIncrement(
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

VOID TdUpdateSibling(
    _In_ PTSC_ENTRY     Sibling,
    _In_ UINTN      OffensiveAddress,
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
        // This means that we reached a potential "After" candidate.
        //

        NT_ASSERT( ARE_VAS_WITHIN_SIBLING_RANGE(Sibling->Before.Address, OffensiveAddress) );

        if ( Sibling->After.Address == 0 ) {

            //
            // Assign detected address.
            //
            if (Sibling->Before.Address > OffensiveAddress) {
                Sibling->After = Sibling->Before;
                Sibling->Before.Address = OffensiveAddress;
                Sibling->Before.Count = 0;
                Sibling->Before.TimeStamp = 0;
                Hit = &Sibling->Before;
            } else {
                Sibling->After.Address = OffensiveAddress;
                Hit = &Sibling->After;
            }

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
        UINT64 Difference = abs((int)(TimeStamp.QuadPart - Sibling->Before.TimeStamp) );

        if ( Sibling->Difference == 0 ) {
            Sibling->Difference = Difference;
        } else {

            UINT64 CurrentDifference   = abs((int)(Difference - Sibling->Difference));
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

    TdHitIncrement(Hit, TimeStamp);
}

VOID 
TdDiscontinueSiblingAssesment(
    _Out_ PTSC_ENTRY Sibling
    )
{
    HvmTrimTsdForQuantum( FALSE );
    TdClearSibling( Sibling );
}

//
// First dummy detection of a timming attack.
//
BOOLEAN
TdIsTimmingCandidate(
    _In_ PTSC_ENTRY Sibling
    )
{
    BOOLEAN IsSyncThresholdOk  = FALSE; 
    BOOLEAN IsSkipsThresholdOk = FALSE; 
    BOOLEAN IsCountThresholdOk = FALSE; 
    BOOLEAN IsTimmingCandidate = FALSE; 

    if (Sibling->Difference == 0) { return FALSE; }

    if (IS_ORPHAN_SIBLING( Sibling ) ) { return FALSE; }

    //
    //  Any average up to this should be discarded.
    //
    if (Sibling->Difference > 0x50000) {
        TdDiscontinueSiblingAssesment( Sibling );
        return FALSE;
    }

    //
    // Count threshold ensures that we, at least, went over 255 retries.
    //
    IsCountThresholdOk = Sibling->Before.Count > 255;

    //
    // Difference threshold synchronizes the number of sibling addresses we fetched.
    //
    IsSyncThresholdOk = abs((int)(Sibling->Before.Count - Sibling->After.Count)) < 2;

    //
    // Skips threshold ensures that we didn't had more than 5% of fetches flushed.
    //
    IsSkipsThresholdOk = Sibling->Skips < max((Sibling->Before.Count / 20), 1);

    IsTimmingCandidate = ( IsSyncThresholdOk &&
                           IsSkipsThresholdOk &&
                           IsCountThresholdOk );

    //
    //  If it is not already a timming candidate then discontinue the assesment
    //  for the Sibling.
    //
    if (!IsTimmingCandidate && IsCountThresholdOk) {
        TdDiscontinueSiblingAssesment( Sibling );
    }

    return IsTimmingCandidate;
}

PTSC_ENTRY TdCreateSibling(
    _In_ PTSC_ENTRY     Head,
    _In_ UINTN      OffensiveAddress,
    _In_ UINTN       TscHash,
    _In_ ULARGE_INTEGER TimeStamp
    )
{
    PTSC_ENTRY Sibling = TdGetSiblingSlot(Head);

    Sibling->TscHash          = TscHash;
    Sibling->Before.Address   = OffensiveAddress;
    Sibling->Before.Count     = 1;
    Sibling->Before.TimeStamp = TimeStamp.QuadPart;

    return Sibling;
}

//
// [SG] TODO: Move to mmu.c.
//
VOID 
TdUnmapAddress(
    _In_ PUINTN Address
)
{
    MmuUnmapIoPage(PAGE_ALIGN( Address ));
}

//
// [SG] TODO: Move to mmu.c.
//
PUINTN
TdMapAddress(
    _In_ UINTN Cr3,
    _In_ UINTN Address
)
{
    PHYSICAL_ADDRESS CodePa = { 0 };
    PUINT8 CodePage = NULL;
    PUINT8 Page = PAGE_ALIGN( Address );

    CodePa = MmuGetPhysicalAddress( Cr3, Page );

    if (0 == CodePa.QuadPart) {
        //
        // [SG] TODO: Send "can't map" trace.
        //
        return NULL;
    }

    CodePage = (PUINT8)MmuMapIoPage( CodePa, FALSE );

    if ( NULL == CodePage ) {
        //
        // [SG] TODO: Send "can't map" trace.
        //
        return NULL;
    }

    return (PUINTN)((UINTN)CodePage + BYTE_OFFSET( Address ));
}

BOOLEAN
TdAreMemoryReferencesWithinRange(
    _In_ PUINT8 Address,
    _In_ UINTN  Size
)
{
    ud_t Udis;

    ud_init( &Udis );
    ud_set_input_buffer( &Udis, Address, Size );

#ifdef _WIN64
    ud_set_mode( &Udis, 64 );
#else
    ud_set_mode( &Udis, 32 );
#endif

    ud_set_syntax( &Udis, UD_SYN_INTEL );


    while (ud_disassemble( &Udis )) {
        
        if ( Udis.operand[0].type == UD_OP_MEM || 
             Udis.operand[1].type == UD_OP_MEM || 
             Udis.operand[2].type == UD_OP_MEM ) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOLEAN 
TdCopySiblingMemory(
    _In_    PTSC_ENTRY Sibling,
    _Inout_ PUINT8 Code,
    _Out_   PUINTN SiblingDistance
    )
{
    PUINTN StartAddress = NULL;
    PUINTN EndAddress = NULL;
    BOOLEAN Result;
    BOOLEAN IsSamePage;
    UINTN Distance = 0;

    //
    // [SG] TODO: At this point reading from VMCS doesn't incur on performance
    //            but it would be better to save it along with the TSC entry.
    //
    UINTN Cr3 = VmReadN( GUEST_CR3 );

    IsSamePage = PAGE_ALIGN( Sibling->Before.Address ) == 
                 PAGE_ALIGN( Sibling->After.Address );

    Distance = Sibling->After.Address - Sibling->Before.Address;

    NT_ASSERT( Distance < SIBLING_DISTANCE_LIMIT );

    StartAddress = TdMapAddress( Cr3, Sibling->Before.Address );

    if (NULL == StartAddress ) {
        Result = FALSE;
        goto RoutineExit;
    }

    if (FALSE == IsSamePage) {

        EndAddress = TdMapAddress( Cr3, Sibling->After.Address );

        if ( NULL == EndAddress ) {
            Result = FALSE;
            goto RoutineExit;
        }

        RtlCopyMemory( Code, StartAddress, Distance - BYTE_OFFSET( EndAddress ) );
        RtlCopyMemory( Code, PAGE_ALIGN( EndAddress ), BYTE_OFFSET( EndAddress ) );

    } else {
        RtlCopyMemory( Code, StartAddress, Distance );
    }

    Result = TRUE;
    *SiblingDistance = Distance;

RoutineExit:

    if (StartAddress) {
        TdUnmapAddress( StartAddress );
    }

    if (FALSE == IsSamePage && EndAddress) {
        TdUnmapAddress( EndAddress );
    }

    return Result;
}

BOOLEAN 
TdIsTimmingAttack(
    _In_ PTSC_ENTRY Sibling
    )
{
    UINT8 Code[ SIBLING_DISTANCE_LIMIT ] = { 0 };

    UINTN Distance;

    if ( TdCopySiblingMemory( Sibling,
                              (PUINT8) &Code,
                              &Distance )) {
        return TdAreMemoryReferencesWithinRange( Code, Distance + 1 );
    }

    return FALSE;
}

BOOLEAN
TdProcessTscEvent(
    _In_ PTSC_ENTRY Head,
    _In_ UINTN OffensiveAddress,
    _In_ ULARGE_INTEGER TimeStamp
    )
{
    UINTN ProcessId;
    UINTN ThreadId;
    UINTN TscHash;
    PTSC_ENTRY Sibling;
    BOOLEAN IsTimmingAttack = FALSE;

    ProcessId = (UINTN) PsGetCurrentProcessId();
    ThreadId  = (UINTN) PsGetCurrentThreadId();

    //
    //  Build the PID:TID Hash.
    //
    TscHash = (ProcessId << TSC_HASH_BITS_HIGH) | ThreadId;

    Sibling = TdFindSibling( Head, TscHash, OffensiveAddress );

    if (Sibling) {
        TdUpdateSibling( Sibling, OffensiveAddress, TimeStamp );

    } else {
        Sibling = TdCreateSibling( Head, 
                                   OffensiveAddress,
                                   TscHash,
                                   TimeStamp );
    }

    if (!TdIsTimmingCandidate( Sibling ) ) {
        return FALSE;
    }

    IsTimmingAttack = TdIsTimmingAttack( Sibling );
    
    // 
    //  This will be the trigger of a detection.
    //
    if (IsTimmingAttack) {
        //
        // [SG] TODO: Refactor to TraceRootInformation.
        //
        RtlPostMailboxTrace( &gSecureMailbox,
                             TRACE_LEVEL_INFORMATION,
                             TRACE_IOA_ROOT,
                             "Timing Attack Found (ProcessId = %p)\n",
                             ProcessId );

        TdTimmingAttackNotify( ProcessId, ThreadId );

    } else {

        //
        //  Prevent subsequent exists for the remaining time slice.
        //
        HvmTrimTsdForQuantum( FALSE );

        RtlPostMailboxTrace( &gSecureMailbox,
                             TRACE_LEVEL_INFORMATION,
                             TRACE_IOA_ROOT,
                             "Timing Candidate Excluded (ProcessId = %p, ThreadId = %p)\n",
                             ProcessId,
                             ThreadId );

        TdTimmingFalsePositiveNotify( ProcessId, ThreadId );
        TdClearSibling( Sibling );
    }

    return IsTimmingAttack;
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
VmRdtscEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS Registers
    )
{
    ULARGE_INTEGER TimeStamp = { 0 };

    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Registers->Rdx = TimeStamp.HighPart;
    Registers->Rax = TimeStamp.LowPart;

    TdProcessTscEvent( Local->TscHits, Registers->Rip, TimeStamp );
    InstrRipAdvance( Registers );
}

VOID
VmRdtscpEmulate(
    _In_ PVCPU_CONTEXT Local,
    _In_ PGP_REGISTERS Registers
    )
{
    LARGE_INTEGER Processor = { 0 };
    ULARGE_INTEGER TimeStamp = { 0 };

    Processor.QuadPart = __readmsr(IA32_TSC_AUX);
    TimeStamp.QuadPart = __readmsr(IA32_TSC);

    Registers->Rdx = TimeStamp.HighPart;
    Registers->Rax = TimeStamp.LowPart;
    Registers->Rcx = Processor.LowPart;

    TdProcessTscEvent( Local->TscHits, Registers->Rip, TimeStamp );
    InstrRipAdvance( Registers );
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
