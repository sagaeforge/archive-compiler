
#include <Module/nString.h>
#include <Module/nStringAry.h>

bool
String_FileAllWrite(const nStringAry_t* pSelf, FILE* pFile)
{
  int _i;
  for (_i = 0; _i < pSelf->m_Length; _i++) {
    fputws(StringAry_get(pSelf, _i)->m_Value, pFile);
    if (_i != pSelf->m_Length - 1)
      fputws(L"\n", pFile);
  }
  return true;
}
