
#include "ProgramManager.h"
#include "String.h"
#include "StringLib.h"
#include <stdio.h>

void
test()
{
  printf("tesewqdwqt");
}

#define Test(FuncPtr) _Generic((FuncPtr), void (*)() : test())

int
main(int argc, char const* argv[])
{
  ProgramManager_Init();
  ProgramManager_ProgramAgumentsSet(argc, argv);
  Application.ProgramStart();

  // // String 모듈 기능 테스트
  // String* strE1 = String("test test test");
  // String* strE2 = String("a");

  // String* Temp = StringMethod.Trim(strE1);

  // printf("%S\n", strE1->Value);
  // printf("%S\n", strE2->Value);
  // printf("%S", Temp->Value);

  void (*test2)();

  Test(test2);

  Application.ProgramQuit();
  return 0;
}
