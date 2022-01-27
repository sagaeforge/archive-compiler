
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAryDestructor(StringAry* Self)
{
  StringAry Ary = *Self;
  if (Ary->Length != 0) {
    StringAryNode** ptr =
      (StringAryNode**)MemoryCreate(sizeof(StringAryNode*) * Ary->Length);

    StringAryNode* node = Ary->Values;
    int i;
    for (i = 0; node != NULL; i++) {
      ptr[i] = node;
      node = node->Next;
    }
    for (i = 0; i < Ary->Length; i++)
      MemoryRemove(ptr[i]);
    MemoryRemove(ptr);
  }

  (*Self)->Values = NULL;
  (*Self)->Length = 0;

  MemoryRemove(Self);
  (*Self) = NULL;
}
