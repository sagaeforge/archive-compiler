
#ifndef __PUBLIC_APPLICATION_OBJECT__
#define __PUBLIC_APPLICATION_OBJECT__

#include "Private_Object.h"

// clang-format off

bool Object_Compare(Object* Self, Object* Obj);

#define Object_GetData(DataType, Instance) \
	_Generic((Instance), Object : *((DataType *) (Instance)->m_Value.m_Value1d))

#define Object(Instance)                                               \
	_Generic((Instance),																								 \
		default : __SystemObject(Instance))(Instance)

#define Boxing(DataType) ((DataType (*)(Object)) __ObjectBoxingSearch(#DataType))
#define UnBoxing(DataType) ((DataType (*)(Object)) __ObjectUnBoxingSearch(#DataType))

FP_Func __ObjectBoxingSearch(const char *pDataType);
FP_Func __ObjectUnBoxingSearch(const char *pDataType);


#endif