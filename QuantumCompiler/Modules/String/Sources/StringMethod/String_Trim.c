
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

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

  Length len = Self->Length - space_Front - space_Rear;
  wcs temp = __WcsCreate(len);
  for (i = 0; i < len; i++)
    temp[i] = Self->Value[i + space_Front];
  temp[i] = '\0';
  return String(temp);
}
