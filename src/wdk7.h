#ifndef __WDK7_H__
#define __WDK7_H__

#include <ntdef.h>

#ifdef WDK7
#define _In_
#define _In_z_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _Success_(X)
#define _IRQL_raises_   __drv_raisesIRQL
#define _Dispatch_type_ __drv_dispatchType
#define _When_(r, c)
#define _In_reads_bytes_(s)
#define _Out_writes_bytes_(s)
#define _Out_writes_bytes_to_(s, b)

#define MdlMappingNoExecute     0x40000000

typedef struct _PHYSICAL_MEMORY_RUN {
    ULONG BasePage;
    ULONG PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR {
    ULONG NumberOfRuns;
    ULONG NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

#endif 

#endif
