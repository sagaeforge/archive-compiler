
#include "Chs.h"
#include "Private_String.h"
#include "Private_StringLib.h"
#include "ProgramManager.h"

String*
String_Right(String* Self, Length Length)
{
  if (Length >= Self->Length)
    return String(Self);

  return String_Extract(Self, Self->Length - Length, Self->Length);
}
