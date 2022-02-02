
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
#include <StringLib.h>
#include <Types/DataType.h>

/*
  해야할 것

  JSON에서 사용할 수 있는 String lib 메소드 추가

  FileAllRead,
  FileAllWrite,
  StringAryToString
  String_IndexAt

  TODO 일단 구현할 메소드들은 구현함. 테스트 해볼것.

  TODO String 모듈의 매개변수 이름 통일
*/

int
main(int argc, char const* argv[])
{
  Application_Initialized(argc, argv);
  // Application.ProcessEvent[ProcessEvent_Awake].AddListener(test);
  Application.ApplicationStart();

  StringAry ary = StringAry(3, String("ABC"), String("GEF"), String("HIJ"));
  String str = toString(ary, String("\n"));

  printf("%S", str->Value);

  Application.ApplicationQuit();
  return 0;
}