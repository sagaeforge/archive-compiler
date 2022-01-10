
#include "ProgramManager.h"
#include "String.h"
#include "StringLib.h"
#include <stdio.h>

int
main(int argc, char const* argv[])
{
  ProgramManager_Init();
  Application.ProgramStart();

  String* str = String("NUGUNGA123, NUGUNGA456, NUGUNGA789, NUGUNGA159");
  // StringAry* Ary = StringAry(1, StringMethod.SubString(str, String(", ")));

  // int i;
  // for (i = 0; i < Ary->Length; i++) {
  //   printf("%S\n", StringAry_Get(Ary, i)->Value);
  // }

  Application.ProgramQuit();
  return 0;
}
