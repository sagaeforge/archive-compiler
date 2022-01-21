
#include "Types/DataType.h"

// 멀티 쓰레드 환경에서 사용을 주의할 것
DataTypeInfo g_DataType;

const DataTypeInfo*
DataType_Find(const char* pDataType)
{
  int i;
  // 시스템 자료형 탐색
  for (i = 0; i != DataType_SystemDataTypeNone; i++)
    if (strcmp(g_SystemDataTypeTable[i].m_Name, pDataType) == 0) {
      g_DataType.m_Option = DataTypeIsSystem;
      g_DataType.m_SystemInfo = (SystemDataTypeInfo*)&g_SystemDataTypeTable[i];
      return &g_DataType;
    }

  // 커스텀 자료형 탐색
  for (i = 0; i != DataType_CustumDataTypeNone; i++)
    if (strcmp(g_CustumDataTypeTable[i].m_Name, pDataType) == 0) {
      g_DataType.m_Option = DataTypeIsCustum;
      g_DataType.m_CustemInfo = (CustumDataTypeInfo*)&g_CustumDataTypeTable[i];
      return &g_DataType;
    }

  // TODO Exception 처리
  return NULL;
}