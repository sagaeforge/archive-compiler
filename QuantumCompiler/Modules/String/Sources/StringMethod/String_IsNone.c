
#include "Private_String.h"
#include "ProgramManager.h"

bool
String_IsNone(String* Self)
{
  return ~Self->Policy & StringPolicy_Null;
}
