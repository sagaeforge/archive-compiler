
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Object.h>
#include <Private_GarbageCollection.h>
#include <String.h>
#include <StringAry.h>
#include <Types/DataType.h>

/*
  해야할 것

  Exception 처리를 일괄히 적용해줘야함.


  JSON에서 사용할 수 있는 String lib 메소드 추가

  FileAllRead,
  FileAllWrite,
  StringAryToString


*/

int
main(int argc, char const* argv[])
{
  Application_Initialized(argc, argv);
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

  String str = String("ABCDEFGHIJKLMNOPQRTWVUXYZ ABCDEFGHIJKLMNOPQRTWVUXYZ");
  printf("%S\n", str->Value);
  // StringAry Ary = StringMethod.Split(str, String(" "));

  // int i;
  // for (i = 0; i < Ary->Length; i++) {
  //   printf("%S\n", StringAryMethod.Get(Ary, i)->Value);
  // }

  // struct DataType DT;
  // printf("\n%lu", sizeof(DT));

  Exception(ERROR, "가나다");

  // char chs = '\0';
  // Object* obds = Object(chs);
  // Test2(&a);

  // trsads(test3213());

  // void* ptr = Object(int)(23);

  printf("%lu", sizeof(long));

  Application.ApplicationQuit();
  return 0;
}