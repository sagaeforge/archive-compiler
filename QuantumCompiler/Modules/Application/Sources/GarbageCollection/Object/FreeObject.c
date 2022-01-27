
#include <Application.h>
#include <Private_GarbageCollection.h>

void
FreeObject(Object Ref)
{
  // TODO Exception 처리
  // 만약에 Ref가 Object의 주소가 아닐경우

  if (Ref == NULL)
    return;

  // index 구하기
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    if (Ref == Application.Member.GarbageCollection_ObjectTable.Value[i])
      break;
  }
  if (i == ObjectMaxLength)
    // TODO Exception 처리
    return;

  Application.Member.GarbageCollection_ObjectTable.IsUsed[i] = false;
  Excute_MemorySet(Application.Member.GarbageCollection_ObjectTable.Value[i],
                   0,
                   1,
                   sizeof(Object_t));
  Application.Member.GarbageCollection_ObjectTable.UsedObjectLength--;
}