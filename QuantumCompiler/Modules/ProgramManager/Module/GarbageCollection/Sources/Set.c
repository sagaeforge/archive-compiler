
#include "Private_GarbageCollection.h"

static int
MemoryCheck(void* Obj, Length Length)
{
  MemoryInfo info = Info(Obj);

  // GarbageCollection이 관리하고 있는 메모리가 아니라면
  if (!info.IsFound)
    return 1;

  // 메모리 할당보다 크게 사용하려는 경우
  if (info.Memory.Length < Length)
    return 2;

  // 수정할 수 없는 메모리
  if (info.Memory.Policy == MemoryPolicy_Const)
    return 3;

  return 0;
}

void
MemorySet(void* Src, int value, Length WordSize, Length Length)
{
  Length *= WordSize;
  if (MemoryCheck(Src, Length) != 0)
    // TODO Exception 처리
    return;

  Private_MemorySet(Src, value, WordSize, Length);
}

void
Private_MemorySet(void* Src, int value, Length WordSize, Length Length)
{
  char* a = (char*)Src;
  char* b = (char*)&value;
  const char* backup = b;

  int i = 0;
  while (i < Length) {
    int j;
    for (j = 0; j < WordSize; a++, b++, i++, j++)
      *a = *b;
    b = (char*)backup;
  }
}