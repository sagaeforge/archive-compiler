
#include <Application.h>
#include <Exception.h>
#include <Private_GarbageCollection.h>

void
FreeObject(Object Ref)
{
  if (Ref == NULL)
    return;

  // index 구하기
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    if (Ref == Application.Member.GarbageCollection_ObjectTable.Value[i])
      break;
  }

  if (i == ObjectMaxLength) {
    Exception(ERROR, "할당한 Object 메모리 위치가 아닙니다. [Ref:%p]", Ref);
    return;
  }

  Application.Member.GarbageCollection_ObjectTable.IsUsed[i] = false;
  Excute_MemorySet(Application.Member.GarbageCollection_ObjectTable.Value[i],
                   0,
                   1,
                   sizeof(Object_t));
  Application.Member.GarbageCollection_ObjectTable.UsedObjectLength--;
}