
#include "Object.h"

// clang-format off

const SystemDataTypeInfo g_SystemDataTypeTable[] = {
//  DataTypeName            | DataTypeCode                    | DataType_WordSize           | Policy  | Constructor | Destructor  | Boxing                                         | UnBoxing 
  { "char",                   DataType_Char,                    sizeof(char),                 0,        NULL,         NULL,         (FP_Func)__Object_Boxing_Char,                   (FP_Func)__Object_UnBoxing_Char },
  { "unsigned char",          DataType_U_Char,                  sizeof(unsigned char),        0 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_U_Char,                 (FP_Func)__Object_UnBoxing_U_Char },
  { "short",                  DataType_Short,                   sizeof(short),                0,        NULL,         NULL,         (FP_Func)__Object_Boxing_Short,                  (FP_Func)__Object_UnBoxing_Short },
  { "unsigned short",         DataType_U_Short,                 sizeof(unsigned short),       0 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_U_Short,                (FP_Func)__Object_UnBoxing_U_Short },
  { "int",                    DataType_Int,                     sizeof(int),                  0,        NULL,         NULL,         (FP_Func)__Object_Boxing_Int,                    (FP_Func)__Object_UnBoxing_Int },
  { "unsigned int",           DataType_U_Int,                   sizeof(unsigned int),         0 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_U_Int,                  (FP_Func)__Object_UnBoxing_U_Int },
  { "long",                   DataType_Long,                    sizeof(long),                 0,        NULL,         NULL,         (FP_Func)__Object_Boxing_Long,                   (FP_Func)__Object_UnBoxing_Long },
  { "unsigned long",          DataType_U_Long,                  sizeof(unsigned long),        0 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_U_Long,                 (FP_Func)__Object_UnBoxing_U_Long },
  { "long long",              DataType_Long_Long,               sizeof(long long),            0,        NULL,         NULL,         (FP_Func)__Object_Boxing_LongLong,               (FP_Func)__Object_UnBoxing_LongLong },
  { "unsigned long long",     DataType_U_Long_Long,             sizeof(unsigned long long),   0 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_U_LongLong,             (FP_Func)__Object_UnBoxing_U_LongLong },
  { "float",                  DataType_Float,                   sizeof(float),                1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Float,                  (FP_Func)__Object_UnBoxing_Float },
  { "double",                 DataType_Double,                  sizeof(double),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double,                 (FP_Func)__Object_UnBoxing_Double },
  { "char*",                  DataType_Ptr_Char,                sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Char,               (FP_Func)__Object_UnBoxing_Ptr_Char },
  { "char**",                 DataType_Double_Ptr_Char,         sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Char,        (FP_Func)__Object_UnBoxing_Double_Ptr_Char },
  { "unsigned char*",         DataType_Ptr_U_Char,              sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_U_Char,             (FP_Func)__Object_UnBoxing_Ptr_U_Char },
  { "unsigned char**",        DataType_Double_Ptr_U_Char,       sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_U_Char,      (FP_Func)__Object_UnBoxing_Double_Ptr_U_Char },
  { "short*",                 DataType_Ptr_Short,               sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Short,              (FP_Func)__Object_UnBoxing_Ptr_Short },
  { "short**",                DataType_Double_Ptr_Short,        sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Short,       (FP_Func)__Object_UnBoxing_Double_Ptr_Short },
  { "unsigned short*",        DataType_Ptr_U_Short,             sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_U_Short,            (FP_Func)__Object_UnBoxing_Ptr_U_Short },
  { "unsigned short**",       DataType_Double_Ptr_U_Short,      sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_U_Short,     (FP_Func)__Object_UnBoxing_Double_Ptr_U_Short },
  { "int*",                   DataType_Ptr_Int,                 sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Int,                (FP_Func)__Object_UnBoxing_Ptr_Int },
  { "int**",                  DataType_Double_Ptr_Int,          sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Int,         (FP_Func)__Object_UnBoxing_Double_Ptr_Int },
  { "unsigned int*",          DataType_Ptr_U_Int,               sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_U_Int,              (FP_Func)__Object_UnBoxing_Ptr_U_Int },
  { "unsigned int**",         DataType_Double_Ptr_U_Int,        sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_U_Int,       (FP_Func)__Object_UnBoxing_Double_Ptr_U_Int },
  { "long*",                  DataType_Ptr_Long,                sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Long,               (FP_Func)__Object_UnBoxing_Ptr_Long },
  { "long**",                 DataType_Double_Ptr_Long,         sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Long,        (FP_Func)__Object_UnBoxing_Double_Ptr_Long },
  { "unsigned long*",         DataType_Ptr_U_Long,              sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_U_Long,             (FP_Func)__Object_UnBoxing_Ptr_U_Long },
  { "unsigned long**",        DataType_Double_Ptr_U_Long,       sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_U_Long,      (FP_Func)__Object_UnBoxing_Double_Ptr_U_Long },
  { "long long *",            DataType_Ptr_Long_Long,           sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Long_Long,          (FP_Func)__Object_UnBoxing_Ptr_Long_Long },
  { "long long **",           DataType_Double_Ptr_Long_Long,    sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Long_Long,   (FP_Func)__Object_UnBoxing_Double_Ptr_Long_Long },
  { "unsigned long long *",   DataType_Ptr_U_Long_Long,         sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_U_Long_Long,        (FP_Func)__Object_UnBoxing_Ptr_U_Long_Long },
  { "unsigned long long **",  DataType_Double_Ptr_U_Long_Long,  sizeof(void *),               1 | 4,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_U_Long_Long, (FP_Func)__Object_UnBoxing_Double_Ptr_U_Long_Long },
  { "float *",                DataType_Ptr_Float,               sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Float,              (FP_Func)__Object_UnBoxing_Ptr_Float },
  { "float **",               DataType_Double_Ptr_Float,        sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Float,       (FP_Func)__Object_UnBoxing_Double_Ptr_Float },
  { "double *",               DataType_Ptr_Double,              sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Double,             (FP_Func)__Object_UnBoxing_Ptr_Double },
  { "double **",              DataType_Double_Ptr_Double,       sizeof(void *),               1,        NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Double,      (FP_Func)__Object_UnBoxing_Double_Ptr_Double },
  { "void *",                 DataType_Ptr_Void,                sizeof(void *),               1 | 8,    NULL,         NULL,         (FP_Func)__Object_Boxing_Ptr_Void,               (FP_Func)__Object_UnBoxing_Ptr_Void },
  { "void **",                DataType_Double_Ptr_Void,         sizeof(void *),               1 | 8,    NULL,         NULL,         (FP_Func)__Object_Boxing_Double_Ptr_Void,        (FP_Func)__Object_UnBoxing_Double_Ptr_Void },
  { "void ***",               DataType_Triple_Ptr_Void,         sizeof(void *),               1 | 8,    NULL,         NULL,         (FP_Func)__Object_Boxing_Triple_Ptr_Void,        (FP_Func)__Object_UnBoxing_Triple_Ptr_Void },
  { "Unknown",                DataType_SystemDataTypeNone,      0,                            0,        NULL,         NULL,         NULL,                                            NULL},
};

const CustumDataTypeInfo g_CustumDataTypeTable[] = {
//  DataTypeName            | DataTypeCode                    | DataType_WordSize           | Policy  | Constructor | Destructor  | Boxing                                         | UnBoxing 
  { "bool",                   DataType_Bool,                    sizeof(bool),                 0,        NULL,         NULL,         NULL,                                            NULL},
  { "Unknown",                DataType_CustumDataTypeNone,      0,                            0,        NULL,         NULL,         NULL,                                            NULL},
};