
#ifndef __PUBLIC_APPLICATION_DATATYPE_OBJECT__
#define __PUBLIC_APPLICATION_DATATYPE_OBJECT__

#include "DataType.h"

#include <stdarg.h>

#pragma pack(push, 1)

typedef struct
{
  DataTypeInfo_t* m_Info;
  void* m_Value;
} Object_t, *Object;

// clang-format on
#pragma pack(pop)

#endif