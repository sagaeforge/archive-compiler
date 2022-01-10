
#include "Private_String.h"
#include "ProgramManager.h"

void
String_UnConst(String* Self)
{
  Self->Policy &= ~StringPolicy_Const;
}
