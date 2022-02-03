
#include <Private_StringAry.h>
#include <Private_StringLib.h>

bool
String_FileAllWrite(StringAry Self, FILE* pFile)
{
  int i;
  for (i = 0; i < Self->Length; i++) {
    fputws(StringAry_Get(Self, i)->Value, pFile);
    if (i != Self->Length - 1)
      fputws(L"\n", pFile);
  }
  return true;
}