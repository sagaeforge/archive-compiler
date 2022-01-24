
#include "Chs.h"
#include "Private_String.h"
#include "Private_StringLib.h"

static bool
_IsSpaceChs(wchar_t ch)
{
  return (ch >= 9 && ch <= 13) || ch == 32;
}

String*
String_Trim(String* Self)
{
  int i, space_Front = 0, space_Rear = 0;
  for (i = 0; i < Self->Length; i++)
    if (!_IsSpaceChs(Self->Value[i]))
      break;
    else
      space_Front++;
  if (space_Front == Self->Length)
    return String("");

  for (i = Self->Length - 1; i >= 0; i--)
    if (!_IsSpaceChs(Self->Value[i]))
      break;
    else
      space_Rear++;

  // TODO 검사
  return String_Extract(Self, space_Front, Self->Length - space_Rear);
}
