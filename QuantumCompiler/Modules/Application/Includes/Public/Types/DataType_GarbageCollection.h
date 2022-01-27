
#ifndef __PUBLIC_APPLICATION_DATATYPE_GARBAGECOLLECTION__
#define __PUBLIC_APPLICATION_DATATYPE_GARBAGECOLLECTION__

#define ObjectMaxLength 256
#define MemoryMaxLength 1024
// clang-format off
#pragma pack(push, 1)

#include <Types/DataType_Object.h>

enum MemoryPolicy
{
  MemoryPolicy_None         = 0,
  MemoryPolicy_Const        = (1 << 0),
  MemoryPolicy_NoDestructor = (1 << 1),
  MemoryPolicy_SystemMemory = (1 << 2)
};


typedef struct
{
  DataTypeInfo_t*       m_TypeInfo;
  enum MemoryPolicy     m_Policy;
  void*                 m_Value;
  Length_t              m_Length;
} Memory_t, *Memory;

typedef struct _MemoryPage {
  Length_t              UsedMemoryLength;
  Memory_t              Nodes[MemoryMaxLength];
  struct _MemoryPage*   Next;
} MemoryPage_t, *MemoryPage;

#pragma pack(pop)
#endif