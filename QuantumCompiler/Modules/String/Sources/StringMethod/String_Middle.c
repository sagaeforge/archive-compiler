
#include "Chs.h"
#include "Private_String.h"
#include "Private_StringLib.h"
#include "ProgramManager.h"

String*
String_Middle(String* Self, Index Start, Index Count)
{
  // TODO 길이 검사
  Length len = Count - Start + 1;
  if (Start + len >= Self->Length)
    return String(Self);

  return String_Extract(Self, Start, Start + Count);
}