
#include "Types/DataType.h"

// 멀티 쓰레드 환경에서 사용을 주의할 것
DataTypeInfo_t g_DataType;

const DataTypeInfo_t*
DataType_Find(const char* pDataType)
{
  int i;
  for (i = 0; i < DataType_None; i++) {
    if (strcmp(pDataType, g_DataTypeTable[i].m_Name) == 0)
      return &g_DataTypeTable[i];
  }

  // TODO Exception 처리
  return NULL;
}