
#include "Private_String.h"

bool
String_Contains(String Self, String Value)
{
  return String_Count(Self, Value) > 0;
}
