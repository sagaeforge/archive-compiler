
#include "Private_String.h"
#include "ProgramManager.h"

String*
String_UnConst(const String* Self)
{
  return (String*)Self;
}
