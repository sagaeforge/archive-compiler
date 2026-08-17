
#include <Application.h>
#include <Exception.h>
#include <Private_GarbageCollection.h>

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

  Exception(ERROR,
            "현재 사용할 수 있는 Object의 개수보다 많습니다. [%u/%u]",
            i,
            Application.Member.GarbageCollection_ObjectTable.UsedObjectLength);
  return NULL;
}