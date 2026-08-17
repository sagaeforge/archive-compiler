
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <Private_JsonAry.h>
#include <StringAry.h>

bool
JSONAry_Destructor(JSONAry* pSelf)
{
  if (*pSelf == NULL)
    return false;

  if ((*pSelf)->m_Length != 0)
    JSONAry_Clear((*pSelf));

  (*pSelf)->m_Length = 0;
  // 부모에서 원소 제거
  if ((*pSelf)->m_Parent.m_Object != NULL) {
    if ((*pSelf)->m_Parent.IsObject) {
      // TODO 이거 구현하려면 JSON에서 이 원소를 찾고, 그 필드이름을 알아내야함.
      // 즉 함수가 필요함.
    } else {
      // TODO 이거 구현하려면 JSONAry에서 이 원소를 찾고, 그 원소 번호를
      // 알아내야함. 즉 함수가 필요함.
    }
  }

  MemoryRemove((*pSelf));
  return true;
}