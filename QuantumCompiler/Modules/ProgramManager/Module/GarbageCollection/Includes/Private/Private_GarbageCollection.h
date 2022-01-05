
#ifndef __PRIVATE_GARBAGECOLLECTION__
#define __PRIVATE_GARBAGECOLLECTION__

#include "ProgramManager.h"

// clang-format off
void        Clear         ();
void       *GetMemory     (MemoryPosition Position);
MemoryInfo  Info          (void *Obj);
void        GC_Append     (void *Obj, Length Length);
void        GC_Remove     (void *Obj);
bool        Policy        (void *Obj, MemoryPolicy Policey);
void        Policy_Append (void *Obj, MemoryPolicy Policey);
void        Policy_Remove (void *Obj, MemoryPolicy Policey);
MemoryPage *PageGet       (Index Index);
MemoryPage *EmptyPageGet  ();
// clang-format on

#endif