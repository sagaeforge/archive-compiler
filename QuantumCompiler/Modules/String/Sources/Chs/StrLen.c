
#include <Chs.h>

Length_t
__StrLen(void* pObj, Length_t pWordSize)
{
  // WordSize를 기준으로 비교함.
  const char* a = pObj;

  int Cnt = 0;
  while (true) {
    int NullCnt = 0;
    int i;
    for (i = 0; i < pWordSize; i++, a++)
      if (*a == '\0')
        NullCnt++;

    if (NullCnt == pWordSize)
      return Cnt;
    Cnt++;
  }
}