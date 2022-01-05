
#include "ProgramManager.h"
#include "String.h"
#include <stdio.h>
#include <wchar.h>

#define Test(Value) _Generic((Value), char * : "Test", char[] : "Test2")

void Test2(String a) {}

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  // Manager.Awake.AddListener(StringModule_Awake);
  Application.ProgramStart();
  // double i = -3.14;
  // // long long d = 9022222222222;

  wcs test = L"가나디";
  String *bab1 = String("Test");
  String *bab2 = String(test);

  printf("%S", StringMethod.Join(bab1, bab2)->Value);
  // printf("%s", Test("abs"));
  // // String *bab2 = String(d);
  // // printf("%S\n", bab1->Value);
  // // printf("%S\n", bab2->Value);
  // // int i = 0;
  // // printf("%s", Test(i));

  Application.ProgramQuit();
  return 0;
}
