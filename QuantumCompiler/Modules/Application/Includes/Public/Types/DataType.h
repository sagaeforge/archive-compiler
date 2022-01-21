
#ifndef __PUBLIC_APPLICATION_DATATYPE__
#define __PUBLIC_APPLICATION_DATATYPE__

#include "Delegate.h"
#include "Private_DataType.h"

#include <string.h>

typedef uint32_t Length_t;
typedef uint32_t Index_t;
typedef uint8_t BitMask;

// clang-format off
#define cast(DataType, Instance) (*(DataType*)&(Instance))
#define type(Instance)                                                         \
  _Generic(Instance,                                                           \
    _Bool : "bool",                                                            \
    Object : "Object",                                                         \
    default : SystemType(Instance)                                             \
  )
#define TypeCompare(Instance1, Instance2) (strcmp(type(Instance1), type(Instance2)) == 0)

typedef void (*Constructor)(void*);
typedef void (*Destructor)(void*);

typedef enum
{
  DataType_Char,
  DataType_U_Char,
  DataType_Short,
  DataType_U_Short,
  DataType_Int,
  DataType_U_Int,
  DataType_Long,
  DataType_U_Long,
  DataType_Long_Long,
  DataType_U_Long_Long,
  DataType_Float,
  DataType_Double,
  DataType_Ptr_Char,
  DataType_Double_Ptr_Char,
  DataType_Ptr_U_Char,
  DataType_Double_Ptr_U_Char,
  DataType_Ptr_Short,
  DataType_Double_Ptr_Short,
  DataType_Ptr_U_Short,
  DataType_Double_Ptr_U_Short,
  DataType_Ptr_Int,
  DataType_Double_Ptr_Int,
  DataType_Ptr_U_Int,
  DataType_Double_Ptr_U_Int,
  DataType_Ptr_Long,
  DataType_Double_Ptr_Long,
  DataType_Ptr_U_Long,
  DataType_Double_Ptr_U_Long,
  DataType_Ptr_Long_Long,
  DataType_Double_Ptr_Long_Long,
  DataType_Ptr_U_Long_Long,
  DataType_Double_Ptr_U_Long_Long,
  DataType_Ptr_Float,
  DataType_Double_Ptr_Float,
  DataType_Ptr_Double,
  DataType_Double_Ptr_Double,
  DataType_Ptr_Void,
  DataType_Double_Ptr_Void,
  DataType_Triple_Ptr_Void,
  DataType_SystemDataTypeNone
} SystemDataTypeCode;

typedef enum
{
  DataTypeOption_TypeIsValue = 0,
  DataTypeOption_TypeIsReference = (1 << 0),
  DataTypeOption_TypeIsNone = (1 << 1),
  DataTypeOption_Unsigned = (1 << 2),
  DataTypeOption_Generic = (1 << 3),
  DataTypeOption_None = (1 << 4),

  DataTypeIsSystem = (1 << 14),
  DataTypeIsCustum = (1 << 15)
} DataTypeOption;

typedef enum
{
  DataType_Bool,
  DataType_CustumDataTypeNone
} CustumDataTypeCode;

typedef struct
{
  char*               m_Name;
  SystemDataTypeCode  m_Code;
  Length_t            m_WordSize;
  DataTypeOption      m_Type;
  Constructor         m_Constructor;
  Destructor          m_Destructor;
  FP_Func             m_Boxing;
  FP_Func             m_UnBoxing;
} SystemDataTypeInfo;

typedef struct
{
  char*               m_Name;
  CustumDataTypeCode  m_Code;
  Length_t            m_WordSize;
  DataTypeOption      m_Type;
  Constructor         m_Constructor;
  Destructor          m_Destructor;
  FP_Func             m_Boxing;
  FP_Func             m_UnBoxing;
} CustumDataTypeInfo;

typedef struct
{
  DataTypeOption        m_Option;
  union {
    SystemDataTypeInfo *m_SystemInfo;
    CustumDataTypeInfo *m_CustemInfo;
  };
} DataTypeInfo;

const DataTypeInfo* DataType_Find(const char *pDataType);

// clang-format on

extern const SystemDataTypeInfo g_SystemDataTypeTable[];
extern const CustumDataTypeInfo g_CustumDataTypeTable[];

#endif