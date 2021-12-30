
#include "Private_String.h"
#include "ProgramManager.h"

static bool _IsLower(wchar_t ch) { return L'a' <= ch && ch <= L'z'; }

String *String_ToUpper(String *Self) {
  wcs temp = __WcsCreate(Self->Length);

  int i;
  for (i = 0; i < Self->Length; i++)
    if (_IsLower(Self->Value[i]))
      temp = 'A' + (Self->Value[i] - 'a');
    else
      temp = Self->Value[i];
  temp[i] = '\0';
  return String(temp);
}
