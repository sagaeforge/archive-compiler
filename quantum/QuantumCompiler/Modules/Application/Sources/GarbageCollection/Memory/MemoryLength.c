
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

Length_t
Excute_MemoryLength(const void* pObj)
{
  MemoryPage page;
  Index_t index;
  Memory info = GarbageCollection_Find(pObj, &page, &index);
  return info->m_Length;
}

Length_t
MemoryLength(const void* pObj)
{
  return Excute_MemoryLength(pObj);
}