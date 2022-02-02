
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>

String
String_ToString_StringAry(StringAry Value, String ReplaceWord)
{
  String str = String("");
  int i;
  for (i = 0; i < Value->Length; i++) {
    String_Append(str, StringAry_Get(Value, i));
    String_Append(str, ReplaceWord);
  }

  return str;
}