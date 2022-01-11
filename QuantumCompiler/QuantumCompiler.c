
#include "ProgramManager.h"
#include "String.h"
#include "StringLib.h"
#include <stdio.h>

int
main(int argc, char const* argv[])
{
  ProgramManager_Init();
  ProgramManager_ProgramAgumentsSet(argc, argv);
  Application.ProgramStart();

  // String 모듈 기능 테스트

  Application.ProgramQuit();
  return 0;
}
