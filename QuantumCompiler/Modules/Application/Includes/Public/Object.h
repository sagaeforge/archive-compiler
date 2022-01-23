
#ifndef __PUBLIC_APPLICATION_OBJECT__
#define __PUBLIC_APPLICATION_OBJECT__

#include "Private_Object.h"

#define Object_GetData(DataType, Instance)                                     \
  _Generic((Instance), Object : *((DataType*)(Instance)->m_Value.m_Value1d))

// clang-format off
#define Obj(Instance)                                                          \
  _Generic((Instance),                                                         \
  _Bool    : NULL,                                                             \
  default  : __SystemObject(Instance))                                         \
  (Instance)

#define Boxing(DataType) ((Object (*)(DataType)) __ObjectBoxingSearch(#DataType))
#define UnBoxing(DataType) ((DataType (*)(Object)) __ObjectUnBoxingSearch(#DataType))

bool Object_Compare(Object* Self, Object* Obj);
Func_t __ObjectBoxingSearch(const char *pDataType);
Func_t __ObjectUnBoxingSearch(const char *pDataType);


#endif