
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

struct _ErrorData
{
  int ErrCode;
  MemoryInfo info;
};

static struct _ErrorData
MemoryCheck(void* Obj1)
{
  struct _ErrorData Err = {
    0,
  };

  // TODO Exception 처리
  MemoryInfo info1 = Info(Obj1);

  // 등록되지 않은 메모리 정의
  if (!info1.IsFound) {
    Err.ErrCode = 1;
  } else {
    // 프로그램이 종료될 때 까지 삭제될 수 없는 메모리
    if (info1.Memory.Policy == MemoryPolicy_NoDestructor)
      Err.ErrCode = 2;
  }
  Err.info = info1;

  return Err;
}

void
GC_Remove(void* ptr)
{
  struct _ErrorData Check = MemoryCheck(ptr);
  if (Check.ErrCode != 0)
    // TODO Exception 처리
    return;

  MemoryPage* page = PageGet(Check.info.Position.PageIndex);
  page->Datas[Check.info.Position.MemoryIndex].Position = NULL;
  page->Datas[Check.info.Position.MemoryIndex].Length = 0;
  page->Datas[Check.info.Position.MemoryIndex].Policy = MemoryPolicy_None;

  int i;
  for (i = Check.info.Position.MemoryIndex; i < page->UsedMemoryLength; i++) {
    Private_MemorySwap(
      &page->Datas[i], &page->Datas[i + 1], sizeof(page->Datas[i]));
  }

  page->UsedMemoryLength--;
  Application.Member.GarbageCollection_UsedMemoryLength--;
}