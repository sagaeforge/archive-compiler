
#include "Application.h"
#include "Private_GarbageCollection.h"

Object
GetObject(DataTypeInfo* pInfo, ObjectValue pValue)
{
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    if (!Application.Member.GarbageCollection_ObjectTable.IsUsed[i]) {
      Application.Member.GarbageCollection_ObjectTable.Value[i]->m_Info = pInfo;
      Application.Member.GarbageCollection_ObjectTable.Value[i]->m_Value =
        pValue;
      Application.Member.GarbageCollection_ObjectTable.UsedObjectLength++;
      return Application.Member.GarbageCollection_ObjectTable.Value[i];
    }
  }

  // TODO Exception 처리
  // 오브젝트의 최대 생성 개수보다 많습니다.
  // 오브젝트를 자료구조 내부에서 사용할 때는 ObjectValue를 사용하세요.
  // ^- 컴파일 애러
  return NULL;
}