#ifndef __NOBJECT_H__
#define __NOBJECT_H__

#include <DataType/nObject.h>

#define nObject(DataType)                                                      \
  (Object_ptr(*)(nDataType)) nObject_BoxingSearch(#DataType)
Func_t nObject_BoxingSearch(const Chs_t pDataType);

#define unObject(DataType)                                                     \
  (DataType(*)(nObject_ptr)) nObject_UnBoxingSearch(#DataType)
Func_t nObject_UnBoxingSearch(const Chs_t pDataType);

extern nObjectDataTypeTableNode_t g_ObjectDataTypeTable[];

#endif // __NOBJECT_H__