
#include "ProgramManager.h"
#include "StringLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define Test(Value) _Generic((Value), int : "Test")

void Test2(String a) {}

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Manager.Awake.AddListener(StringModule_Awake);
  Manager.Method.ProgramStart();
  double i = -3.14;
  // long long d = 9022222222222;
  String *bab1 = String(i);

  printf("%g", ValueOf(Digit, bab1));
  // String *bab2 = String(d);
  // printf("%S\n", bab1->Value);
  // printf("%S\n", bab2->Value);
  // int i = 0;
  // printf("%s", Test(i));

  Manager.Method.ProgramQuit();
  return 0;
}
