
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

static bool _IsUpper(wchar_t ch) { return L'A' <= ch && ch <= L'Z'; }

String *String_ToLower(String *Self) {
  wcs temp = __WcsCreate(Self->Length);

  int i;
  for (i = 0; i < Self->Length; i++)
    if (_IsUpper(Self->Value[i]))
      temp[i] = 'a' + (Self->Value[i] - 'A');
    else
      temp[i] = Self->Value[i];
  temp[i] = '\0';
  return String(temp);
}
