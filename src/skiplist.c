/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    skiplist.c

Abstract:

    This file implements the skip list data structure.

Environment:

--*/

#include "skiplist.h"

#define RTL_SKIPLIST_DEFAULT_SEED 48015UL

FORCEINLINE
UINT32 
SklRandom(
    _In_ PRTL_SKIPLIST_TABLE Table
    )
{
    UINT64 t;

    NT_ASSERT( Table != NULL );

    Table->KissState.x = 314527869 * Table->KissState.x + 1234567;
    Table->KissState.y ^= Table->KissState.y << 5;
    Table->KissState.y ^= Table->KissState.y >> 7;
    Table->KissState.y ^= Table->KissState.y << 22;
    t = 4294584393ULL * Table->KissState.z + Table->KissState.c;
    Table->KissState.c = t >> 32;
    Table->KissState.z = (ULONG)t;

    return Table->KissState.x + Table->KissState.y + Table->KissState.z;
}

FORCEINLINE
INT32
SklRandomLevel(
    _In_ PRTL_SKIPLIST_TABLE Table
    )
{
    INT32 Level = 1;

    NT_ASSERT( Table != NULL );

    //
    //  Probabilistic random level generator.
    //
    while (SklRandom( Table ) < (MAXLONG / 2) && Level < RTL_SKIPLIST_MAX_LEVEL) {
        ++Level;
    }

    return Level;
}

FORCEINLINE
PRTL_SKIPLIST_NODE 
SklLookup(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ BOOLEAN *Found,
    _Inout_opt_ PRTL_SKIPLIST_NODE *Update
    )
{
    RTL_GENERIC_COMPARE_RESULTS Result;
    PRTL_SKIPLIST_NODE Node;
    INT32 Idx;

    NT_ASSERT( Table != NULL );

    *Found = FALSE;
    Node = Table->Head;

    for (Idx = Table->Level; Idx >= 0; Idx--) {

        while (NULL != Node->Forward[Idx]) {
            Result = Table->CompareRoutine( Table,
                                            Buffer,
                                            Node->Forward[Idx]->Data );

            if (Result == GenericGreaterThan) {
                Node = Node->Forward[Idx];
            }
            else {

                //
                //  Pugh algorithm requires two calls to the compare routine.
                //  Optimize by doing the test for equality in the same pass
                //  as traversal.
                //
                if (Result == GenericEqual) {
                    *Found = TRUE;
                }

                break;
            }
        }

        if (ARGUMENT_PRESENT( Update )) {
            Update[Idx] = Node;
        }
    }

    return Node;
}

_Must_inspect_result_
PVOID
RtlLookupElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer
    )
{
    PRTL_SKIPLIST_NODE Node;
    BOOLEAN Found;

    NT_ASSERT( Table != NULL );

    Node = SklLookup( Table, Buffer, &Found, NULL )->Forward[0];
    return (Found == TRUE) ? Node->Data : NULL;
}

PVOID
RtlInsertElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    )
{
    PVOID Data;
    PRTL_SKIPLIST_NODE Node;
    PRTL_SKIPLIST_NODE Update[RTL_SKIPLIST_MAX_LEVEL + 1];
    UINT32 NodeSize;
    INT32 Level;
    INT32 Idx;
    BOOLEAN Found;

    NT_ASSERT( Table != NULL );

    if (ARGUMENT_PRESENT( NewElement )) {
        *NewElement = FALSE;
    }

    Node = SklLookup( Table, Buffer, &Found, Update )->Forward[0];

    if (Found == TRUE) {
        return Node->Data;
    }

    //
    //  The real secret of magic lies in the performance.
    //
    Level = SklRandomLevel( Table );

    NodeSize = sizeof( RTL_SKIPLIST_NODE )
            + (sizeof( PRTL_SKIPLIST_NODE ) * Level);

    Node = Table->AllocateRoutine( Table, NodeSize );

    if (Node == NULL) {
        return NULL;
    }

    Data = Table->AllocateRoutine( Table, BufferSize );

    if (Data == NULL) {
        Table->FreeRoutine( Table, Node );
        return NULL;
    }

    if (ARGUMENT_PRESENT( NewElement )) {
        *NewElement = TRUE;
    }

    RtlCopyMemory( Data, Buffer, BufferSize );

    Node->Data = Data;
    Table->Size++;

    if (Level > Table->Level) {
        for (Idx = Table->Level + 1; Idx <= Level; Idx++) {
            Update[Idx] = Table->Head;
        }

        Table->Level = Level;
    }

    Node->Backward = Update[0];

    if (Update[0]->Forward[0] != NULL) {
        Update[0]->Forward[0]->Backward = Node;
    }

    for (Idx = 0; Idx <= Level; Idx++) {   
        Node->Forward[Idx] = Update[Idx]->Forward[Idx];
        Update[Idx]->Forward[Idx] = Node;
    }

    return Data;
}

BOOLEAN
RtlDeleteElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer
    )
{
    PRTL_SKIPLIST_NODE Node;
    PRTL_SKIPLIST_NODE Update[RTL_SKIPLIST_MAX_LEVEL + 1];
    INT32 Idx;
    BOOLEAN Found;

    NT_ASSERT( Table != NULL );

    Node = SklLookup( Table, Buffer, &Found, Update )->Forward[0];

    if (!Found) {
        return FALSE;
    }

    for (Idx = 0; Idx < Table->Level; Idx++) {
        if (Update[Idx]->Forward[Idx] != Node) {
            break;
        }

        Update[Idx]->Forward[Idx] = Node->Forward[Idx];
    }

    if (Node->Backward != NULL) {
        Node->Backward->Forward[0] = Node->Forward[0];
    }

    if (Node->Forward[0] != NULL) {
        Node->Forward[0]->Backward = Node->Backward;
    }

    Table->FreeRoutine( Table, Node->Data );
    Table->FreeRoutine( Table, Node );
    --Table->Size;

    while (Table->Level > 0) {
        if (Table->Head->Forward[Table->Level - 1] != NULL) {
            break;
        }

        --Table->Level;
    }

    return TRUE;
}

BOOLEAN
RtlIsGenericTableEmptySkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table
    )
{
    NT_ASSERT( Table != NULL );
    return ((Table->Size) ? (FALSE) : (TRUE));
}

LONGLONG
RtlNumberGenericTableElementsSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table
    )
{
    NT_ASSERT( Table != NULL );
    return Table->Size;
}

PVOID
NTAPI
RtlEnumerateGenericTableSkiplist (
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID *RestartKey
    )
{
    PRTL_SKIPLIST_NODE Node;
    PRTL_SKIPLIST_NODE Restart;

    NT_ASSERT( Table != NULL );

    if (ARGUMENT_PRESENT( RestartKey )) {
        Restart = (PRTL_SKIPLIST_NODE)*RestartKey;
        *RestartKey = Node = Restart->Forward[0];
    }
    else {
        Node = Table->Head->Forward[0];
    }

    return (Node ? Node->Data : NULL);
}

PVOID
RtlEnumerateGenericTableWithoutSplayingSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID *RestartKey
    )
{
    PRTL_SKIPLIST_NODE Node;
    PRTL_SKIPLIST_NODE Restart;

    NT_ASSERT( Table != NULL );

    if (ARGUMENT_PRESENT( RestartKey )) {
        Restart = (PRTL_SKIPLIST_NODE)*RestartKey;
        *RestartKey = Node = Restart->Forward[0];
    }
    else {
        Node = Table->Head->Forward[0];
    }

    return (Node ? Node->Data : NULL);
}

_Must_inspect_result_
BOOLEAN
RtlInitializeGenericTableSkiplist(
    _Out_ PRTL_SKIPLIST_TABLE Table,
    _In_ PRTL_SKIPLIST_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_SKIPLIST_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_SKIPLIST_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    )
{
    PRTL_SKIPLIST_NODE Head;
    ULONG Seed = RTL_SKIPLIST_DEFAULT_SEED;
    UINT32 HeadSize;
    INT32 Idx;

    //
    //  Seed the random number generator.
    //
    Table->KissState.x = 123456789;
    Table->KissState.y = RtlRandomEx( &Seed );
    Table->KissState.z = 43219876;
    Table->KissState.c = 6543217;

    HeadSize = sizeof( RTL_SKIPLIST_NODE )
            + (sizeof( PRTL_SKIPLIST_NODE ) * RTL_SKIPLIST_MAX_LEVEL);

    Head = AllocateRoutine( Table, HeadSize );

    if (NULL == Head) {
        KdPrint(( "%s: %s Unable to allocate head node.\n", __FILE__, __FUNCTION__ ));
        return FALSE;
    }

    Head->Data = NULL;

    for (Idx = 0; Idx <= RTL_SKIPLIST_MAX_LEVEL; Idx++) {
        Head->Forward[Idx] = NULL;
    }

    Table->Head = Head;
    Table->Level = 0;
    Table->Size = 0;
    Table->Context = TableContext;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;

    return TRUE;
}

/*
PVOID
RtlInsertElementGenericTableFullSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    )
{
}

VOID
RtlDeleteElementGenericTableSkiplistEx(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID NodeOrParent
    )
{
}

PVOID
RtlLookupElementGenericTableFullSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    )
{
}

PVOID
RtlLookupFirstMatchingElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
    )
{
}

PVOID
RtlGetElementGenericTableSkiplist(
    _In_ PRTL_SKIPLIST_TABLE Table,
    _In_ ULONG I
    )
{
}
*/
