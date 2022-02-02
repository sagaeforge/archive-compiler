
#include <GarbageCollection.h>
#include <Private_String.h>

Length_t
String_Count(String Self, String Value)
{
  if (Self->Length < Value->Length)
    return -1;

  int i;
  Length_t sum = 0;
  for (i = 0; i < Self->Length - Value->Length + 1; i++)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i)) {
        sum++;
        i += Value->Length - 1;
      }

  return sum;
}
