
#ifndef __PRIVATE_APPLICATION_GARBAGECOLLECTION__
#define __PRIVATE_APPLICATION_GARBAGECOLLECTION__

#include "Types/DataType_GarbageCollection.h"

// clang-format off
void GarbageCollectionModule_Initialized();


// 오브젝트에 대응하는 함수들
Object GetObject(DataTypeInfo *pInfo, ObjectValue pValue);
void   FreeObject(const Object Ref);


// 조건 검사하지 않고 실행하는 함수들
__attribute__((warn_unused_result)) void*
Excute_MemoryConstructor             (const DataTypeInfo* pInfo);
__attribute__((warn_unused_result)) Constructor
Excute_MemoryConstructor_FP          (const DataTypeInfo* pInfo);
__attribute__((warn_unused_result)) void*
Excute_MemoryConstructorAry          (const DataTypeInfo* pInfo,
                                      const Length_t pLength);
void*    Excute_MemoryDestructor     (const DataTypeInfo *pInfo, 
                                      const void* pObj);
__attribute__((warn_unused_result)) void *
Excute_MemoryCreate                  (const Length_t pLength);
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
void          GarbageCollection_Append        (const void *pObj, const DataTypeInfo* pInfo, const Length_t pLength);
void          GarbageCollection_Remove        (const void *pObj);
const Memory  GarbageCollection_Find          (const void *pObj);
MemoryPage    GarbageCollection_PageGet       (const Index_t pIndex);
MemoryPage    GarbageCollection_EmptyPageGet  ();

#endif