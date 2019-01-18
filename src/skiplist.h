/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    skiplist.h

--*/

#ifndef _SKIPLIST_H_
#define _SKIPLIST_H_

#include <ntifs.h>
#include "wdk7.h"

//
//  Define an appropiate max. level data structures containing up to 64K
//  elements
//
#define RTL_SKIPLIST_MAX_LEVEL 16

//
//  Forwarder definitions for the function pointers.
//
typedef struct _RTL_SKIPLIST_TABLE RTL_SKIPLIST_TABLE, *PRTL_SKIPLIST_TABLE;

typedef
_IRQL_requires_same_
_Function_class_( RTL_SKIPLIST_COMPARE_ROUTINE )
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_SKIPLIST_COMPARE_ROUTINE(
    _In_ struct _RTL_SKIPLIST_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_SKIPLIST_COMPARE_ROUTINE *PRTL_SKIPLIST_COMPARE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_( RTL_SKIPLIST_ALLOCATE_ROUTINE )
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_SKIPLIST_ALLOCATE_ROUTINE(
    _In_ struct _RTL_SKIPLIST_TABLE *Table,
    _In_ CLONG ByteSize
    );
typedef RTL_SKIPLIST_ALLOCATE_ROUTINE *PRTL_SKIPLIST_ALLOCATE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_( RTL_SKIPLIST_FREE_ROUTINE )
__drv_freesMem(Mem)
VOID
NTAPI
RTL_SKIPLIST_FREE_ROUTINE(
    _In_ struct _RTL_SKIPLIST_TABLE *Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    );
typedef RTL_SKIPLIST_FREE_ROUTINE *PRTL_SKIPLIST_FREE_ROUTINE;

typedef struct _RTL_SKIPLIST_NODE {
    PVOID Data;
    struct _RTL_SKIPLIST_NODE *Backward;
    struct _RTL_SKIPLIST_NODE *Forward[1];
} RTL_SKIPLIST_NODE, *PRTL_SKIPLIST_NODE;

typedef struct _RTL_SKIPLIST_TABLE {
    PRTL_SKIPLIST_NODE Head;
    LONG Level;
    ULONG Size;

    struct
    {
        ULONG x;
        ULONG y;
        ULONG z;
        ULONG c;
    } KissState;

    PVOID Context;
    PRTL_SKIPLIST_COMPARE_ROUTINE CompareRoutine;
    PRTL_SKIPLIST_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_SKIPLIST_FREE_ROUTINE FreeRoutine;
} RTL_SKIPLIST_TABLE, *PRTL_SKIPLIST_TABLE;

typedef struct _RTL_SKIPLIST_ITOR {
    struct _RTL_SLIPLIST_TABLE *Table;
    struct _RTL_SKIPLIST_NODE *Node;
} RTL_SKIPLIST_ITOR, *PRTL_SKIPLIST_ITOR;

PVOID
RtlInsertElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );

BOOLEAN
RtlDeleteElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer
    );

_Must_inspect_result_
PVOID
RtlLookupElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer
    );

BOOLEAN
RtlIsGenericTableEmptySkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table
    );

LONGLONG
RtlNumberGenericTableElementsSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table
    );

PVOID
RtlEnumerateGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID *RestartKey
    );

PVOID
RtlEnumerateGenericTableWithoutSplayingSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID *RestartKey
    );

BOOLEAN
RtlInitializeGenericTableSkiplist(
    _Out_ PRTL_SKIPLIST_TABLE Table,
    _In_ PRTL_SKIPLIST_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_SKIPLIST_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_SKIPLIST_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    );

//
//  The follwoing functions still need implementing to round out
//  interface parity with the WDK generic table interface.
//

PVOID
RtlInsertElementGenericTableFullSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    );

VOID
RtlDeleteElementGenericTableSkiplistEx(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID NodeOrParent
    );

PVOID
RtlLookupElementGenericTableFullSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );

PVOID
RtlLookupFirstMatchingElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
    );

PVOID
RtlGetElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ ULONG I
    );

#endif
