
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

struct _ErrorData
{
  int ErrCode;
  MemoryInfo info;
};

static struct _ErrorData
MemoryCheck(void* Obj)
{
  struct _ErrorData Err = {
    0,
  };

  MemoryInfo info = Info(Obj);

  // 중복 정의
  if (info.IsFound) {
    Err.ErrCode = 1;
    return Err;
  }

  Err.info = info;
  return Err;
}

bool
Policy(void* Obj, MemoryPolicy Policy)
{
  struct _ErrorData Data = MemoryCheck(Obj);
  if (Data.ErrCode != 0)
    // TODO Exception 처리
    return false;
  return Data.info.Memory.Policy & Policy;
}

void
Policy_Append(void* Obj, MemoryPolicy Policy)
{
  struct _ErrorData Data = MemoryCheck(Obj);
  if (Data.ErrCode != 0)
    // TODO Exception 처리
    return;
  MemoryPage* page = PageGet(Data.info.Position.PageIndex);
  page->Datas[Data.info.Position.MemoryIndex].Policy |= Policy;
}

void
Policy_Remove(void* Obj, MemoryPolicy Policy)
{
  struct _ErrorData Data = MemoryCheck(Obj);
  if (Data.ErrCode != 0)
    // TODO Exception 처리
    return;
  MemoryPage* page = PageGet(Data.info.Position.PageIndex);
  page->Datas[Data.info.Position.MemoryIndex].Policy &= ~Policy;
}
