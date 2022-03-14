#ifndef __NOBJECT_H__
#define __NOBJECT_H__

#include <DataType/Global.h>
#include <DataType/nString.h>

typedef struct {
  nString_ptr m_DataType;
  union {
    bool Value_bool;
    char Value_char;
    int64_t Value_int;
    uint64_t Value_uint;
    double Value_float;
    void *Value_pointer;
  };
} Object_t, *Object_ptr;

#endif // __NOBJECT_H__