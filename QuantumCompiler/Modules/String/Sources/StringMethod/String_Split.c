
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>

StringAry
String_Split(String pSelf, String pValue)
{
  StringAry Ary = StringAryConstructor(0);
  Length_t Cnt = String_Count(pSelf, pValue);

  if (Cnt == 0) {
    StringAry_Push(Ary, pSelf);
    return Ary;
  }

  Index_t Start = 0;
  int i;
  for (i = 0; i < Cnt; i++) {
    Index_t index = String_IndexFor(pSelf, pValue, i);
    StringAry_Push(Ary, String_Extract(pSelf, Start, index));
    Start = index + pValue->Length;
  }

  Index_t index = String_IndexFor(pSelf, pValue, i - 1) + pValue->Length;
  StringAry_Push(Ary, String_Extract(pSelf, index, pSelf->Length));

  return Ary;
}
