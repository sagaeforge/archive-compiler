
#include "ProgramManager.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Manager.Method.ProgramStart();

  Manager.Method.ProgramQuit();
  return 0;
}
