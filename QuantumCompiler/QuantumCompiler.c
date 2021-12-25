
#include "ProgramManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int v = 1;

void test() {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  printf("Update Call[%02d]: %02d:%02d:%02d\n", v, tm.tm_hour, tm.tm_min,
         tm.tm_sec);
  v++;
}

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Manager.Method.ProgramInit();
  Manager.Update.AddListener(test);
  Manager.Method.ProgramStart();
  sleep(1);

  Manager.Method.ProgramQuit();
  return 0;
}
