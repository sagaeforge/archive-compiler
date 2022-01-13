
#ifndef __PUBLIC_DATATYPES_OBJECT__
#define __PUBLIC_DATATYPES_OBJECT__

#include "DataTypes.h"

typedef struct _Object
{
  const char* DataType;
  const void* Value;
  const Length MemoryLength;
} Object;

typedef struct
{
  /* data */
} ObjectNode;

typedef struct _Obejcts
{
  Length Length;
  ObjectNode Node;
} Objects;

#endif