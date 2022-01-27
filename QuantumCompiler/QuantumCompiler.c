
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <GarbageCollection.h>
#include <Object.h>
#include <Private_GarbageCollection.h>
#include <String.h>
#include <StringAry.h>
#include <Types/DataType.h>

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

  printf("%lu byte\n", sizeof(Application));

  int* a = Constructor(int);

  *a = 50;
  printf("%d\n", *a);

  Object obj = Boxing(int)(50);
  printf("%s\n", g_DataTypeTable[DataType_Int].m_Name);
  int b = UnBoxing(int)(obj);
  printf("%d\n", b);

  setlocale(LC_ALL, "");

  String str = String("ABCDEFGHIJKLMNOPQRTWVUXYZ ABCDEFGHIJKLMNOPQRTWVUXYZ");
  StringAry Ary = StringMethod.Split(str, String(" "));

  int i;
  for (i = 0; i < Ary->Length; i++) {
    printf("%S\n", StringAryMethod.Get(Ary, i)->Value);
  }

  // struct DataType DT;
  // printf("\n%lu", sizeof(DT));

  // char chs = '\0';
  // Object* obds = Object(chs);
  // Test2(&a);

  // trsads(test3213());

  // void* ptr = Object(int)(23);

  printf("%lu", sizeof(long));

  Application.ApplicationQuit();
  return 0;
}
