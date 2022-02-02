
#include <GarbageCollection.h>
#include <Private_String.h>

bool
String_Compare(String Self, String Value)
{
  if (Self->Length != Value->Length)
    return false;

  int i;
  for (i = 0; i < Self->Length; i++)
    if (Self->Value[i] != Value->Value[i])
      return false;
  return true;
}
