#ifndef __DATATYPE_NOBJECT_H__
#define __DATATYPE_NOBJECT_H__

#include <DataType/Global.h>
#include <DataType/nString.h>

typedef union {
  int64_t Value_int;
  uint64_t Value_uint;
  double Value_float;
  void *Value_pointer;
} nObject_t, *nObject_ptr;

typedef struct {
  Chs_t m_DataType;
  Func_t m_Boxing;
  Func_t m_UnBoxing;
} nObjectDataTypeTableNode_t, *nObjectDataTypeTableNode_ptr;

#endif // __NOBJECT_H__