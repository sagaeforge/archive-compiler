
#include "Private_StringLib.h"

double String_ValueOf_Digit(String *Self) {
  // TODO 최적화, 오류 검사
  wcs EndPos = NULL;
  return wcstold(Self->Value, &EndPos);
}
