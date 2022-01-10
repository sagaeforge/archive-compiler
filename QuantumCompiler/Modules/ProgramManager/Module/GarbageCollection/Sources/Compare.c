
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

  return 0;
}

bool
MemoryCompare(void* Obj1, void* Obj2, Length Length)
{
  if (MemoryCheck(Obj1, Obj2, Length) != 0)
    // TODO Exception 처리
    return false;

  char* a = (char*)Obj1;
  char* b = (char*)Obj2;
  int i = 0;
  while (i < Length) {
    if (*a != *b)
      return false;
    a++, b++, i++;
  }
  return true;
}