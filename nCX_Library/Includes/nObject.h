#ifndef __NOBJECT_H__
#define __NOBJECT_H__

#include <DataType/nObject.h>

#define nObject(DataType)                                                      \
  (Object_ptr(*)(nDataType)) nObject_BoxingSearch(#DataType)
Func_t
nObject_BoxingSearch(const Chs_t pDataType);

#define unObject(DataType)                                                     \
  (DataType(*)(nObject_ptr)) nObject_UnBoxingSearch(#DataType)
Func_t
nObject_UnBoxingSearch(const Chs_t pDataType);

#define AddObjectType_Define(DataType)                                         \
  __attribute__((warn_unused_result))                                          \
    nObject_ptr __Object_Boxing_##DataType(DataType pSelf);                    \
  DataType __Object_unBoxing_##DataTYpe(nObject_ptr pSelf);

#define AddObjectType_Implement(DataType)                                      \
  __attribute__((warn_unused_result))                                          \
    nObject_ptr __Object_Boxing_##DataType(DataType pSelf)                     \
  {                                                                            \
    DataType _temp = (DataType)malloc(sizeof(DataType));                       \
    if (!_temp)                                                                \
      NULL;                                                                    \
    *_temp = pSelf;                                                            \
                                                                               \
    nObject_ptr _obj = malloc(sizeof(nObject_t));                              \
    if (!_obj)                                                                 \
      NULL;                                                                    \
    _obj->m_Value = _temp;                                                     \
    return _obj;                                                               \
  }                                                                            \
  DataType __Object_unBoxing_##DataTYpe(nObject_ptr pSelf)                     \
  {                                                                            \
    DataType _temp = *(DataType*)pSelf->m_Value;                               \
    free(pSelf->m_Value);                                                      \
    free(pSelf);                                                               \
    return _temp;                                                              \
  }

extern nObjectDataTypeTableNode_t g_ObjectDataTypeTable[];

#endif // __NOBJECT_H__