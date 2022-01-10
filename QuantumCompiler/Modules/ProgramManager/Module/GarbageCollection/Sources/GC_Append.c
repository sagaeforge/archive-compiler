
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

static int
MemoryCheck(void* Obj1, Length Length)
{
  // TODO Exception 처리
  MemoryInfo info1 = Info(Obj1);

  // 중복 정의
  if (info1.IsFound)
    return 1;

  return 0;
}

void
GC_Append(void* ptr, Length Length)
{
  if (MemoryCheck(ptr, Length) != 0)
    // TODO Exception 처리
    return;

  MemoryPage* page = EmptyPageGet();

  page->Datas[page->UsedMemoryLength].Position = ptr;
  page->Datas[page->UsedMemoryLength].Length = Length;
  page->Datas[page->UsedMemoryLength].Policy = MemoryPolicy_None;
  page->UsedMemoryLength++;
  Application.Member.GarbageCollection_UsedMemoryLength++;
}