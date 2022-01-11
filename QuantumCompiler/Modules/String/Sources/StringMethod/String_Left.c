
#include "Chs.h"
#include "Private_String.h"
#include "Private_StringLib.h"
#include "ProgramManager.h"

// TODO 오류 검사
String*
String_Left(String* Self, Length Length)
{
  if (Length >= Self->Length)
    return String(Self);

  return String_Extract(Self, 0, Length);
}