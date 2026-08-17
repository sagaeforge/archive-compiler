
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

__attribute__((warn_unused_result)) void*
Excute_MemoryConstructor(const DataTypeInfo_t* pInfo)
{
  if (pInfo == NULL)
    return NULL;

  Length_t Length = pInfo->m_WordSize;

  void* ptr = malloc(Length);
  if (ptr == NULL) {
    Exception(ERROR, "메모리 할당에 실패했습니다. [size:%u]", Length);
    return NULL;
  }

  GarbageCollection_Append(ptr, NULL, Length);
  Excute_MemorySet(ptr, 0, 1, Length);
  if (pInfo->m_Constructor != NULL)
    pInfo->m_Constructor(&ptr);

  return ptr;
}

__attribute__((warn_unused_result)) void*
MemoryConstructor(const DataTypeInfo_t* pInfo)
{
  return Excute_MemoryConstructor(pInfo);
}