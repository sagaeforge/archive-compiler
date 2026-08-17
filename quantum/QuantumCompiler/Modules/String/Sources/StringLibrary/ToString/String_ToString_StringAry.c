
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>

String
String_ToString_StringAry(StringAry pValue, String pReplaceWord)
{
  String str = String("");
  int i;
  for (i = 0; i < pValue->m_Length; i++) {
    String_Append(str, StringAry_Get(pValue, i));
    String_Append(str, pReplaceWord);
  }

  return str;
}