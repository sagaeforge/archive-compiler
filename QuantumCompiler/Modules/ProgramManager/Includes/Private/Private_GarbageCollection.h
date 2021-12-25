
#ifndef __PRIVATE_GARBAGECOLLECTION__
#define __PRIVATE_GARBAGECOLLECTION__

#include "ProgramManager.h"

void Clear();
void *Memory(MemoryPosition Position);
MemoryInfo Info(void *Obj);
void GC_Append(void *Obj, Length Length);
void GC_Remove(void *Obj);
bool GC_CreateCheck(void *Obj1, void *Obj2);
bool Policey(void *Obj, MemoryPolicey Policey);
void Policey_Append(void *Obj, MemoryPolicey Policey);
void Policey_Remove(void *Obj, MemoryPolicey Policey);
bool GC_IndexOfExceptionCheck(void *Obj, Length Length);
void *MemoryConstCreate(Length Length);

#endif