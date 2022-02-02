
#include <Chs.h>
#include <Exception.h>
#include <GarbageCollection.h>

wcs
__WcsCreate(Length_t Length)
{
  wcs temp = MemoryCreate(sizeof(wchar_t) * (Length + 1));
  if (temp == NULL) {
    Exception(ERROR,
              "메모리를 생성할 수 없습니다. [size:%lu]",
              sizeof(wchar_t) * (Length + 1));
    return NULL;
  }
  return temp;
}