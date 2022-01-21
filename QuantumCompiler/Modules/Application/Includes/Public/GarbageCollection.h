
#ifndef __PUBLIC_APPLICATION_GARBAGECOLLECTION__
#define __PUBLIC_APPLICATION_GARBAGECOLLECTION__

#include "Types/DataType.h"

#define Constructor(DataType)                                                  \
  (DataType*)MemoryConstructor(DataType_Find(#DataType))

#define ConstructorParam(DataType, Param...)                                   \
  (DataType * (*)(##Param)) MemoryConstructor_FP(DataType_Find(#DataType))

#define Destructor(DataType, Instance)                                         \
  MemoryDestructor(DataType_Find(#DataType), Instance)

// clang-format off
__attribute__((warn_unused_result)) void*
MemoryConstructor              (const DataTypeInfo* pInfo);
__attribute__((warn_unused_result)) Constructor
MemoryConstructor_FP           (const DataTypeInfo* pInfo);
__attribute__((warn_unused_result)) void*
MemoryConstructorAry           (const DataTypeInfo* pInfo,
                                const Length_t pLength);
void*     MemoryDestructor     (const DataTypeInfo *pInfo, 
                                const void* pObj);
__attribute__((warn_unused_result)) void *
MemoryCreate                   (const Length_t pLength);
void      MemorySet            (void* pObj,
                                const int pValue,
                                const Length_t pWordSize,
                                const Length_t pLength);
void      MemoryCopy           (void* pObj,
                                const void* pData,
                                const Length_t pLength);
void      MemoryMove           (void* pObj,
                                const void* pData,
                                const Length_t pLength);
void      MemorySwap           (void* pObj,
                                void*  pData,
                                const Length_t pLength);
bool      MemoryCompare        (const void* pObj1,
                                const void*  pObj2,
                                const Length_t pLength);
Length_t  MemoryLength         (const void* pObj);

#endif