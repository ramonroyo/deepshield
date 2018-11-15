/*++

Copyright (C) ByteHeed. All Rights Reserved

Module Name:

    idxlist.h

--*/

#ifndef _IDXLIST_H_
#define _IDXLIST_H_

#include <ntifs.h>
#include "wdk7.h"

//
//  The lists of free and active nodes are tracked using indices in order to
//  allow copying the IDXLIST table in memory.
//
typedef struct _IDXLIST_NODE {
    ULONG32 Fidx;
    ULONG32 Bidx;
    PVOID Data;
} IDXLIST_NODE;

typedef struct _IDXLIST_TABLE {
    ULONG32 Size;
    IDXLIST_NODE *NodeList;
    ULONG32 NodeFreeList;
    ULONG32 NodeCount;
} IDXLIST_TABLE, *PIDXLIST_TABLE;

//
//  Walk through all allocated entries.
//

#define INDEX_FOREACH(Table, Index)                 \
    for (Index = (Table)->NodeList[0].Fidx;         \
         Index != 0;                                \
         Index = (Table)->NodeList[Index].Fidx)

#define TREE_NODE_FOREACH(Table, Type, Node)        \
    Type Node = NULL;                               \
    for (Node = (Table)->NodeList[0].Fidx;          \
         Node != NULL;                              \
         Node = (Table)->NodeList[Index].Fidx)

VOID
InitializeIdxList(
    _Inout_ IDXLIST_TABLE *Table
    );

VOID
DestroyIdxList(
    _Inout_ IDXLIST_TABLE *Table
    );

ULONG
GetSizeIdxList(
    _In_ IDXLIST_TABLE *Table
    );

ULONG
GetNodeCountIdxList(
    _In_ IDXLIST_TABLE *Table
    );

PVOID
LookupNodeIdxList(
    _In_ IDXLIST_TABLE *Table,
    _In_ ULONG32 Idx
    );

PVOID
GetHeadNodeIdxList(
    _In_ IDXLIST_TABLE *Table
    );

PVOID
GetNextNodeIdxList(
    _In_ IDXLIST_TABLE *Table,
    _In_ ULONG32 Idx
    );

ULONG32
InsertHeadIdxList(
    IDXLIST_TABLE *Table,
    PVOID Data
    );

PVOID
RemoveNodeIdxList(
    IDXLIST_TABLE *Table,
    ULONG32 Idx
    );

PVOID
RemoveHeadIdxList(
    IDXLIST_TABLE *Table
    );

#endif // _IDXLIST_H_
