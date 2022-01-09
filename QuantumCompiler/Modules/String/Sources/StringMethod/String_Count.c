
#include "GarbageCollection.h"
#include "Private_String.h"

// TODO 최적화
static bool
_StringCompare(wcs Ary, String* FindValue, Index Start)
{
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}

Length
String_Count(String* Self, String* Value)
{
  // TODO 조건 검사
  int i;
  Length sum = 0;
  for (i = 0; i < Self->Length - Value->Length + 1; i++)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        sum++;

  return sum;
}
