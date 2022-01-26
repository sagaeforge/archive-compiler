
#include "Object.h"
#include "Application.h"

Func_t
__ObjectBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    // TODO Exception 처리
    return NULL;
  if (Info->m_Boxing != NULL)
    return Info->m_Boxing;
  // TODO Exception 처리
  // 박싱 함수가 없을 때
  return NULL;
}

Func_t
__ObjectUnBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    // TODO Exception 처리
    return NULL;
  if (Info->m_UnBoxing != NULL)
    return Info->m_UnBoxing;
  // TODO Exception 처리
  // 언박싱 함수가 없을 때
  return NULL;
}