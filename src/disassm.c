#include "disassm.h"
#include <capstone.h>


typedef struct _CS_MEM_ALLOCATION
{
    UINTN Size;
    UINT8 Memory[1];
} CS_MEM_ALLOCATION, *PCS_MEM_ALLOCATION;

C_ASSERT(sizeof(CS_MEM_ALLOCATION) == sizeof(void *) * 2);

static PVOID CAPSTONE_API
CapstoneCalloc(
    _In_ UINTN N,
    _In_ UINTN Size
    );

static PVOID CAPSTONE_API
CapstoneMalloc(
    _In_ UINTN Size
    );

static VOID CAPSTONE_API
CapstoneFree(
    _In_opt_ PVOID Pointer
    );

static PVOID CAPSTONE_API
CapstoneRealloc(
    _In_opt_ PVOID Pointer,
    _In_ UINTN Size
    );

static INT CAPSTONE_API
CapstoneVsnprintf(
    PINT8 Buffer,
    UINTN Count,
    const char* Format,
    va_list Arguments
    );

_Out_ __drv_allocatesMem( Mem ) cs_insn*
TdCapstoneAllocateInstruction(
    VOID
    )
{
    cs_insn* Instruction = NULL;

    Instruction = DsAllocatePoolWithTag( NonPagedPool, 
                           sizeof(cs_insn) + sizeof(cs_detail), 
                           'pMdT' ); 

    Instruction->detail = (cs_detail*)(Instruction + 1);
    return Instruction;
}

VOID
TdCapstoneFreeInstruction(
    _In_ __drv_freesMem( Mem ) _Post_invalid_ cs_insn* Instruction
    )
{
    DsFreePoolWithTag( Instruction, 'pMdT' ); 
}

static PVOID CAPSTONE_API 
CapstoneCalloc(
    _In_ UINTN N,
    _In_ UINTN Size
    )
{
    UINTN Total = N * Size;

    PVOID Alloc = CapstoneMalloc( Size );

    if (NULL == Alloc) {
        return NULL;
    }

    return RtlFillMemory( Alloc, Total, 0 );
}

static PVOID CAPSTONE_API 
CapstoneMalloc(
    _In_ UINTN Size
    )
{
    PCS_MEM_ALLOCATION Pool;

    NT_ASSERT( Size );

    Pool = (PCS_MEM_ALLOCATION) DsAllocatePoolWithTag( NonPagedPool,
                                        (UINT32) Size + sizeof(CS_MEM_ALLOCATION),
                                        CS_POOL_TAG);
    if (NULL == Pool) {
        return NULL;
    }

    Pool->Size = Size;
    return Pool->Memory;
}

static VOID CAPSTONE_API 
CapstoneFree(
    //
    // We expect to have _In_ here but we have no guarantees that 
    // null pointers aren't supplied so it will remain as optional
    //
    _In_opt_ PVOID Pointer
    )
{
    //
    // As said before, a defensive check
    //
    if (NULL == Pointer) {
        return;
    }

    DsFreePoolWithTag( CONTAINING_RECORD( Pointer, 
                                          CS_MEM_ALLOCATION,
                                          Memory), 
                       CS_POOL_TAG );
}

static PVOID CAPSTONE_API
CapstoneRealloc(
    _In_opt_ PVOID Pointer,
    _In_ UINTN Size
    )
{
    PVOID NewAllocation = NULL;
    UINTN CurrentSize = 0;
    UINTN SmallerSize = 0;

    if (NULL == Pointer) {
        return CapstoneMalloc( Size );
    }

    NewAllocation = CapstoneMalloc( Size );

    if (NULL == NewAllocation) {
        return NULL;
    }

    CurrentSize = CONTAINING_RECORD( Pointer, CS_MEM_ALLOCATION, Memory )->Size;
    SmallerSize = (CurrentSize < Size) ? CurrentSize : Size;

    RtlCopyMemory( NewAllocation, Pointer, SmallerSize );
    CapstoneFree( Pointer );

    return NewAllocation;
}

/*
//
// Function ported from here: https://github.com/tandasat/cs_driver/blob/master/cs_driver/cs_driver_mm.c
// vsnprintf(). _vsnprintf() is avaialable for drivers, but it differs from
// vsnprintf() in a return value and when a null-terminater is set.
// csdrv_vsnprintf() takes care of those differences.
//
#pragma warning(push)
#pragma warning(disable : 28719)  // Banned API Usage : _vsnprintf is a Banned
                                  // API as listed in dontuse.h for security
                                  // purposes.
static INT CAPSTONE_API 
CapstoneVsnprintf(
    PINT8 Buffer, 
    UINTN Count,
    const char* Format, 
    va_list Arguments
    )
{
    size_t Result = 0;
    CHAR Temporal[ 1024 ];

    NTSTATUS Status = RtlStringCchVPrintfA( Buffer,
                                            Count, 
                                            Format, 
                                            Arguments );

    if (Status == STATUS_BUFFER_OVERFLOW) {
        //
        //  RtlStringCchVPrintfA returns STATUS_BUFFER_OVERFLOW when a string is
        //  truncated, and a null-terminater needs to be added manually.
        //
        Buffer[Count - 1] = '\0';

        //
        //  In case when STATUS_BUFFER_OVERFLOW is returned, the function has to
        //  get and return a number of characters that would have been written.
        //  This attempts so by re-tring the same conversion with temp buffer that
        //  is most likely big enough to complete formatting and get a number of
        //  characters that would have been written.
        //

        Status = RtlStringCchVPrintfA( Temporal,
                                       RTL_NUMBER_OF(Temporal), 
                                       Format, 
                                       Arguments );

        NT_ASSERT( Status == STATUS_SUCCESS );

        Buffer = Temporal;
        Count = 1024;
    }

    Status = RtlStringCchLengthA( Buffer, Count, &Result );
    NT_ASSERT( Status == STATUS_SUCCESS );

    return (INT)Result;
}
#pragma warning(pop)*/

#pragma warning(push)
#pragma warning(disable : 4995)
#pragma warning(disable : 28719)
int CAPSTONE_API 
CapstoneVsnprintf(
    char *buffer,
    size_t count,
    const char *format,
    va_list argptr
    )
{
    int result = _vsnprintf(buffer, count, format, argptr);

    // _vsnprintf() returns -1 when a string is truncated, and returns "count"
    // when an entire string is stored but without '\0' at the end of "buffer".
    // In both cases, null-terminator needs to be added manually.
    if (result == -1 || (size_t)result == count) {
        buffer[count - 1] = '\0';
    }

    if (result == -1) {
        // In case when -1 is returned, the function has to get and return a number
        // of characters that would have been written. This attempts so by retrying
        // the same conversion with temp buffer that is most likely big enough to
        // complete formatting and get a number of characters that would have been
        // written.
        char* tmp = CapstoneMalloc(0x1000);
        if (!tmp) {
            return result;
        }

        result = _vsnprintf_s(tmp, 0x1000, count, format, argptr);
        NT_ASSERT(result != -1);
        CapstoneFree(tmp);
    }

    return result;
}
#pragma warning(pop)

PCS_INSN CsInitialize(
    _Inout_ PCS_HANDLE CsHandle
    )
{
    cs_opt_mem CapstoneSetup = { 0 };
    CS_ERR Error;

    CapstoneSetup.malloc     = CapstoneMalloc;
    CapstoneSetup.calloc     = CapstoneCalloc;
    CapstoneSetup.realloc    = CapstoneRealloc;
    CapstoneSetup.free       = CapstoneFree;
    CapstoneSetup.vsnprintf  = CapstoneVsnprintf;

    Error = cs_option( 0,
                       CS_OPT_MEM, 
                       (size_t) &CapstoneSetup );

    if (CS_ERR_OK != Error) {
        return NULL;
    }

    Error = cs_open( CS_ARCH_X86,
                     CS_MODE_64,
                     CsHandle );

    if (CS_ERR_OK != Error) {
        return NULL;
    }

    Error = cs_option( *CsHandle, 
                       CS_OPT_DETAIL, 
                       TRUE );

    if (CS_ERR_OK != Error) {
        return NULL;
    }

    return cs_malloc( *CsHandle );
}

VOID 
CsDestroy(
    _In_ PCS_HANDLE CsHandle,
    _In_ PCS_INSN   Instruction
    )
{
    CS_ERR Error;

    cs_free( Instruction, 1 );

    Error = cs_close( CsHandle );

    if (CS_ERR_OK != Error) {
        return;
    }

}
