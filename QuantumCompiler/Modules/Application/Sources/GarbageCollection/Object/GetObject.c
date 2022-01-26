
#include "Application.h"
#include "Private_GarbageCollection.h"

Object
GetObject(const DataTypeInfo_t* pInfo, const void* pValue)
{
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    if (!Application.Member.GarbageCollection_ObjectTable.IsUsed[i]) {
      Object temp = Application.Member.GarbageCollection_ObjectTable.Value[i];
      temp->m_Info = (DataTypeInfo_t*)pInfo;
      temp->m_Value = (void*)pValue;
      Application.Member.GarbageCollection_ObjectTable.UsedObjectLength++;
      return Application.Member.GarbageCollection_ObjectTable.Value[i];
    }
  }

  // TODO Exception 처리
  // 오브젝트의 최대 생성 개수보다 많습니다.
  // 오브젝트를 자료구조 내부에서 사용할 때는 ObjectValue_t를 사용하세요.
  // ^- 컴파일 애러
  return NULL;
}