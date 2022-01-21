
#include "Application.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Object.h"
#include "Types/DataType.h"

/*
  해야할 것

  GC 포팅 메모리 페이지가 3개로 구성됨.
  Object 테이블 - Object 구조체만 저장함 있음.
  Object Boxing 테이블 - Object 구조체의 값만 저장함.
  Memory 테이블

*/

int
main(int argc, char const* argv[])
{
  Application_Initialized();
  // Application.ProcessEvent[ProcessEvent_Awake].AddListener(test);
  Application.ApplicationStart();

  printf("%lu byte", sizeof(Application));

  // struct DataType DT;
  // printf("\n%lu", sizeof(DT));

  // char chs = '\0';
  // Object* obds = Object(chs);
  // Test2(&a);

  // trsads(test3213());

  // void* ptr = Object(int)(23);

  Application.ApplicationQuit();
  return 0;
}
