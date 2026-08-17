
#include <Application.h>
#include <Exception.h>
#include <Object.h>

Func_t
__ObjectBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    return NULL;
  if (Info->m_Boxing != NULL)
    return Info->m_Boxing;

  Exception(WARNING, "기본 박싱 함수가 없습니다. [Type: %s]", Info->m_Name);
  return NULL;
}

Func_t
__ObjectUnBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    return NULL;
  if (Info->m_UnBoxing != NULL)
    return Info->m_UnBoxing;

  Exception(WARNING, "기본 언박싱 함수가 없습니다. [Type: %s]", Info->m_Name);
  return NULL;
}