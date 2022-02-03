
#include <Private_StringAry.h>
#include <Private_StringLib.h>

bool
String_FileAllWrite(StringAry pSelf, FILE* pFile)
{
  int i;
  for (i = 0; i < pSelf->Length; i++) {
    fputws(StringAry_Get(pSelf, i)->Value, pFile);
    if (i != pSelf->Length - 1)
      fputws(L"\n", pFile);
  }
  return true;
}