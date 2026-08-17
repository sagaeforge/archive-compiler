
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

void
Excute_MemoryDestructor(const DataTypeInfo_t* pInfo, void* pObj)
{
  if (pInfo->m_Destructor != NULL)
    pInfo->m_Destructor(&pObj);

  Excute_MemorySet(pObj, 0, 1, pInfo->m_WordSize);
  GarbageCollection_Remove(pObj);
}

void
MemoryDestructor(const DataTypeInfo_t* pInfo, void* pObj)
{
  Excute_MemoryDestructor(pInfo, pObj);
}