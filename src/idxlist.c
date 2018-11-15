/*++

Copyright (c) ByteHeed. All Rights Reserved.

Module Name:

    idxlist.c

Abstract:

    Implementation of a IDXLIST to quickly map an index to a pointer.

Environment:

    kernel mode only.

--*/

#include "dsdef.h"
#include <wdm.h>

IDXLIST_NODE EmptyList = { 0, 0, NULL };

VOID
InitializeIdxList(
    _Inout_ IDXLIST_TABLE *Table
    )
{
    Table->Size = 0;
    Table->NodeList = &EmptyList;
    Table->NodeFreeList = 0;
    Table->NodeCount = 0;
}

VOID
DestroyIdxList(
    _Inout_ IDXLIST_TABLE *Table
    )
{
    if (Table->NodeList != &EmptyList) {
        ExFreePoolWithTag( Table->NodeList, 'lIsD' );
        Table->NodeList = &EmptyList;
    }

    Table->Size = 0;
    Table->NodeFreeList = 0;
    Table->NodeCount = 0;
}

ULONG
GetSizeIdxList(
    _In_ IDXLIST_TABLE *Table
    )
{
    return Table->Size;
}

ULONG
GetNodeCountIdxList(
    _In_ IDXLIST_TABLE *Table
    )
{
    return Table->NodeCount;
}

PVOID
LookupNodeIdxList(
    _In_ IDXLIST_TABLE *Table,
    _In_ ULONG32 Idx
    )
{
    return (Idx < Table->Size) ? Table->NodeList[Idx].Data : NULL;
}

PVOID
GetHeadNodeIdxList(
    _In_ IDXLIST_TABLE *Table
    )
{
    return (Table->Size) ?
        Table->NodeList[Table->NodeList[0].Fidx].Data :
        NULL;
}

PVOID
GetNextNodeIdxList(
    _In_ IDXLIST_TABLE *Table,
    _In_ ULONG32 Idx
    )
{
    return (Idx < Table->Size) ?
        Table->NodeList[Table->NodeList[Idx].Fidx].Data :
        NULL;
}

BOOLEAN
ExtendIdxList(
    _Inout_ IDXLIST_TABLE *Table
    )
{
    IDXLIST_NODE *NodeList;
    ULONG32 Size;
    ULONG32 Idx;

    //
    //  The IDXLIST allows storing pointers into a dynamically growing array
    //  for fast lookups.
    //
    
    Size = Table->Size + (PAGE_SIZE / sizeof( IDXLIST_NODE ));

    NodeList = (IDXLIST_NODE*)
        ExAllocatePoolWithTag( NonPagedPool, 
                               Size * sizeof( IDXLIST_NODE ),
                               'lIsD' );
    if (NULL == NodeList) {
        return FALSE;
    }

    Idx = Size;
    while ((Idx--) > Table->Size) {

        NodeList[Idx].Data = NULL;
        NodeList[Idx].Fidx = Table->NodeFreeList;
        Table->NodeFreeList = Idx;
    }

    if (Table->NodeList != &EmptyList) {
        RtlCopyMemory( NodeList,
                       Table->NodeList,
                       Table->Size * sizeof( IDXLIST_NODE ));
        ExFreePoolWithTag( Table->NodeList, 'lIsD' );

    } else {

        NodeList[0].Fidx = 0;
        NodeList[0].Bidx = 0;
        Table->NodeFreeList = 1;
    }

    Table->NodeList = NodeList;
    Table->Size = Size;

    return TRUE;
}

ULONG32
InsertHeadIdxList(
    IDXLIST_TABLE *Table,
    PVOID Data
    )
{
    IDXLIST_NODE *Node;
    ULONG32    Idx;

    if (Table->NodeFreeList == 0 && !ExtendIdxList( Table )) {
        return 0;
    }

    Idx = Table->NodeFreeList;
    Node = &Table->NodeList[Idx];
    Table->NodeFreeList = Node->Fidx;

    Node->Data = Data;
    Node->Fidx = Table->NodeList[0].Fidx;
    Table->NodeList[0].Fidx = Idx;
    Node->Bidx = 0;
    Table->NodeList[Node->Fidx].Bidx = Idx;

    Table->NodeCount++;
    return Idx;
}

PVOID
RemoveNodeIdxList(
    IDXLIST_TABLE *Table,
    ULONG32 Idx
    )
{
    IDXLIST_NODE *Node;
    PVOID Data;

    if (Idx >= Table->Size || 0 == Idx) {
        return NULL;
    }

    Node = &Table->NodeList[Idx];
    if (NULL == Node->Data) {
        return NULL;
    }

    Table->NodeList[Node->Fidx].Bidx = Node->Bidx;
    Table->NodeList[Node->Bidx].Fidx = Node->Fidx;

    Data = Node->Data;
    Node->Data = NULL;
    Node->Fidx = Table->NodeFreeList;
    Table->NodeFreeList = Idx;

    Table->NodeCount--;
    return Data;
}

PVOID
RemoveHeadIdxList(
    IDXLIST_TABLE *Table
    )
{
    return RemoveNodeIdxList( Table, Table->NodeList[0].Fidx );
}