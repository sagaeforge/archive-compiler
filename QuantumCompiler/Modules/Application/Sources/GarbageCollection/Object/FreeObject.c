
#include "Application.h"
#include "Private_Object.h"

void
FreeObject(const Object* Ref)
{
  // TODO Exception 처리
  // 만약에 Ref가 Object의 주소가 아닐경우

  if (Ref == NULL)
    return;

  // index 구하기
  Index_t Index =
    (Index_t)Ref -
    (Index_t)Application.Member.GarbageCollection_ObjectTable.Value;

  Index /= sizeof(Object);

  Application.Member.GarbageCollection_ObjectTable.IsUsed[Index] = false;
  Application.Member.GarbageCollection_ObjectTable.Value[Index]->m_Info = NULL;
  Application.Member.GarbageCollection_ObjectTable.Value[Index]
    ->m_Value.m_Value1d = NULL;
  Application.Member.GarbageCollection_ObjectTable.UsedObjectLength--;
}