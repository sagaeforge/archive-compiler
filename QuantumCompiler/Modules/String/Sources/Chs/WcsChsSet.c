
#include "Chs.h"
#include "GarbageCollection.h"

wcs
__WcsCreate(Length Length)
{
  wcs temp = MemoryCreate(sizeof(wchar_t) * (Length + 1));
  if (temp == NULL) {
    // Warning("메모리를 생성할 수 없습니다. (Size:%lu)",
    //         sizeof(wchar_t) * (Length + 1));
    // TODO Exception 처리
    return NULL;
  }
  return temp;
}