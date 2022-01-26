
#include "Chs.h"

Length_t
__StrLen(void* Obj, Length_t WordSize)
{
  // WordSize를 기준으로 비교함.
  const char* a = Obj;

  int Cnt = 0;
  while (true) {
    int NullCnt = 0;
    int i;
    for (i = 0; i < WordSize; i++, a++)
      if (*a == '\0')
        NullCnt++;

    if (NullCnt == WordSize)
      return Cnt;
    Cnt++;
  }
}