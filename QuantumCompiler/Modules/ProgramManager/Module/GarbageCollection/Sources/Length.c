
#include "Private_GarbageCollection.h"

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

  // 할당된 메모리가 아니라면
  MemoryInfo info = Info(Obj);
  if (!info.IsFound) {
    Err.ErrCode = 1;
    return Err;
  }

  Err.info = info;
  return Err;
}

Length
MemoryLength(void* Obj)
{
  struct _ErrorData data = MemoryCheck(Obj);
  if (data.ErrCode != 0)
    // TODO Exception 처리
    return -1;

  return data.info.Memory.Length;
}