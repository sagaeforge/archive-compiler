
#include "ProgramManager.h"
#include "StringLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define Test(Value) _Generic((Value), String : "Test")

void Test2(String a) {}

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Manager.Awake.AddListener(StringModule_Awake);
  Manager.Method.ProgramStart();
  double i = -3.1415926842378431241212123425436E+29;
  String *bab = String(i);
  printf("%S\n", bab->Value);
  printf("%u\n", bab->Length);

  Manager.Method.ProgramQuit();
  return 0;
}
