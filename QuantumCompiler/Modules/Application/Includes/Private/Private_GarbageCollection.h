
#ifndef __PRIVATE_APPLICATION_GARBAGECOLLECTION__
#define __PRIVATE_APPLICATION_GARBAGECOLLECTION__

#include <Types/DataType_GarbageCollection.h>

// clang-format off
void GarbageCollectionModule_Initialized();

// 조건 검사하지 않고 실행하는 함수들
__attribute__((warn_unused_result)) void*
Excute_MemoryConstructor             (const DataTypeInfo_t* pInfo);
void     Excute_MemoryDestructor     (const DataTypeInfo_t *pInfo, 
                                      void* pObj);
__attribute__((warn_unused_result)) void *
Excute_MemoryCreate                  (const Length_t pLength);
void     Excute_MemoryRemove         (void* pObj);
void     Excute_MemorySet            (void* pObj,
                                      const int pValue,
                                      const Length_t pWordSize,
                                      const Length_t pLength);
void     Excute_MemoryCopy           (void* pObj,
                                      const void* pData,
                                      const Length_t pLength);
void     Excute_MemoryMove           (void* pObj,
                                      const void* pData,
                                      const Length_t pLength);
void     Excute_MemorySwap           (void* pObj,
                                      void*  pData,
                                      const Length_t pLength);
bool     Excute_MemoryCompare        (const void* pObj1,
                                      const void*  pObj2,
                                      const Length_t pLength);
Length_t Excute_MemoryLength         (const void* pObj);

// GC 기능들
void          GarbageCollection_Append        (const void *pObj, const DataTypeInfo_t* pInfo, const Length_t pLength);
void          GarbageCollection_Remove        (const void *pObj);
const Memory  GarbageCollection_Find          (const void *pObj, MemoryPage* Out_pMemoryPage, Length_t *Out_pIndex);
MemoryPage    GarbageCollection_PageGet       (const Index_t pIndex);
MemoryPage    GarbageCollection_EmptyPageGet  ();

#endif