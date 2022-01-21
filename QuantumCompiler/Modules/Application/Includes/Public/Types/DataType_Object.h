
#ifndef __PUBLIC_APPLICATION_DATATYPE_OBJECT__
#define __PUBLIC_APPLICATION_DATATYPE_OBJECT__

#include "DataType.h"

#include <stdarg.h>

#pragma pack(push, 1)

typedef union
{
  void* m_Value1d;
  void** m_Value2d;
  void*** m_Value3d;
} ObjectValue;

typedef struct
{
  DataTypeInfo* m_Info;
  ObjectValue m_Value;
} * Object, Object_t;

// clang-format on
#pragma pack(pop)

#endif