
#include <Chs.h>
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_Middle(String Self, Index_t Start, Index_t End)
{
  // TODO 길이 검사
  Length_t len = End - Start + 1;
  if (Start + len >= Self->Length)
    return String(Self);

  return String_Extract(Self, Start, End);
}