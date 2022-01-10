
#ifndef __PUBLIC_DATATYPES_GARBAGECOLLECTION__
#define __PUBLIC_DATATYPES_GARBAGECOLLECTION__

#include "DataTypes.h"

#define MemoryMaxLength 1024

#pragma pack(push, 1)

typedef enum
{
  // clang-format off
  MemoryPolicy_None             = 0,
  MemoryPolicy_Const            = (1 << 0),
  MemoryPolicy_NoDestructor     = (1 << 1),
  MemoryPolicy_SystemMemory     = (1 << 2)
  // clang-format on
} MemoryPolicy;

typedef struct
{
  Index PageIndex;
  Index MemoryIndex;
} MemoryPosition;

typedef struct
{
  void* Position;
  MemoryPolicy Policy;
  Length Length;
} Memory;

typedef struct
{
  bool IsFound;
  Memory Memory;
  MemoryPosition Position;
} MemoryInfo;

typedef struct _MemoryPage
{
  Length UsedMemoryLength;
  Memory Datas[MemoryMaxLength];
  MemoryInfo Info;
  struct _MemoryPage* Next;
} MemoryPage;

#pragma pack(pop)

#endif