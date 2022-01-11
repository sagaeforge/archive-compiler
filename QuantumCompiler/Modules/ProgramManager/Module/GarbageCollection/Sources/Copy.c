
#include "Private_GarbageCollection.h"

static int
MemoryCheck(void* Obj1, void* Obj2, Length Length)
{
  // TODO Exception 처리
  MemoryInfo info1 = Info(Obj1);
  MemoryInfo info2 = Info(Obj2);

  // GarbageCollection이 관리하고 있는 메모리가 아니라면
  if (!info1.IsFound || !info2.IsFound)
    return 1;

  // 메모리 할당 보다 크게 사용하려는 경우
  if (info1.Memory.Length < Length)
    return 2;
  if (info2.Memory.Length < Length)
    return 3;

  // 수정할 수 없는 메모리
  if (info1.Memory.Policy == MemoryPolicy_Const)
    return 4;

  return 0;
}

void
MemoryCopy(void* Src, void* Data, Length Length)
{
  if (MemoryCheck(Src, Data, Length))
    // TODO exception 처리
    return;

  Private_MemoryCopy(Src, Data, Length);
}

void
Private_MemoryCopy(void* Src, void* Data, Length Length)
{
  char* a = (char*)Src;
  char* b = (char*)Data;

  int i = 0;
  while (i < Length) {
    *a = *b;
    i++, a++, b++;
  }
}