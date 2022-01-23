
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
    Object : "object",                                                         \
    default : SystemType(Instance)                                             \
  )
#define TypeCompare(Instance1, Instance2) (strcmp(type(Instance1), type(Instance2)) == 0)

typedef void (*Constructor_t)(void**);
typedef void (*Destructor_t)(void*);

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
  DataType_Bool,
  DataType_None
} DataTypeCode_t;

typedef enum
{
  DataTypeOption_TypeIsValue      = 0,
  DataTypeOption_TypeIsReference  = (1 << 0),
  DataTypeOption_TypeIsNone       = (1 << 1),
  DataTypeOption_Unsigned         = (1 << 2),
  DataTypeOption_Generic          = (1 << 3),
  DataTypeOption_None             = (1 << 4),
} DataTypeOption_t;

#pragma pack(push, 1)
typedef struct
{
  char*             m_Name;
  int               m_Code;
  Length_t          m_WordSize;
  DataTypeOption_t  m_Type;
  Constructor_t     m_Constructor;
  Destructor_t      m_Destructor;
  Func_t            m_Boxing;
  Func_t            m_UnBoxing;
} DataTypeInfo_t;
#pragma pack(pop)

const DataTypeInfo_t* DataType_Find(const char *pDataType);

// clang-format on

extern const DataTypeInfo_t g_DataTypeTable[];
#ifdef DEBUG
extern const DataTypeInfo_t* Debug_DataTypeTable;
#endif

#endif