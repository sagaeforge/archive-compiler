#ifndef __GARBAGECOLLECTION__
#define __GARBAGECOLLECTION__

#include "DataTypes.h"

// clang-format off
void *MemoryCreate  (Length Length);
void  MemoryRemove  (void **ptr);
void  MemorySet     (void *Src, int value, Length WordSize, Length Length);
void  MemoryCopy    (void *Src, void *Data, Length Length);
void  MemoryMove    (void *Src, void *Data, Length Length);
void  MemorySwap    (void *Src, void *Data, Length Length);
bool  MemoryCompare (void *Obj1, void *Obj2, Length Length);
Length MemoryLength  (void *Obj);
// clang-format on

#endif
