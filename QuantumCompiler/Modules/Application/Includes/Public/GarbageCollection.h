
#ifndef __PUBLIC_APPLICATION_GARBAGECOLLECTION__
#define __PUBLIC_APPLICATION_GARBAGECOLLECTION__

#include "Types/DataType.h"

#define Constructor(DataType)                                                  \
  (DataType*)MemoryConstructor(DataType_Find(#DataType))

#define Destructor(DataType, Instance)                                         \
  MemoryDestructor(DataType_Find(#DataType), Instance)

// clang-format off
__attribute__((warn_unused_result)) void*
MemoryConstructor              (const DataTypeInfo_t* pInfo);
void      MemoryDestructor     (const DataTypeInfo_t *pInfo, 
                                void* pObj);
__attribute__((warn_unused_result)) void *
MemoryCreate                   (const Length_t pLength);
void      MemoryRemove         (void* pObj);
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