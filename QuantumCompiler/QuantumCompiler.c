
#include "ProgramManager.h"
#include "String.h"
#include <stdio.h>
#include <wchar.h>

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  // Manager.Awake.AddListener(StringModule_Awake);
  Application.ProgramStart();
  // double i = -3.14;
  // // long long d = 9022222222222;

  // wcs test = L"abc";
  // clang-format off
  String *bab1 = String("abfdDSADASFGskmv@#!$@!$sklfdl3124121s");
  // clang-format on
  // String *bab2 = String(test);

  printf("%S", StringMethod.Middle(bab1, 2, 3)->Value);

  // printf("%s", Test("abs"));
  // // String *bab2 = String(d);
  // // printf("%S\n", bab1->Value);
  // // printf("%S\n", bab2->Value);
  // // int i = 0;
  // // printf("%s", Test(i));

  Application.ProgramQuit();
  return 0;
}
