
#include "ProgramManager.h"
#include "String.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define Test(Value) _Generic((Value), String : "Test")

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Manager.Awake.AddListener(StringModule_Awake);
  Manager.Method.ProgramStart();
  String base = {
      0,
  };
  String *str = String(&base);
  // String *str = String("");
  printf("%p\n", str->Value);

  printf("%p\n", &StringMethod);

  Manager.Method.ProgramQuit();
  return 0;
}
