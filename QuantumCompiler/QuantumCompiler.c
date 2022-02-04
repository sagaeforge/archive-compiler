
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Json.h>
#include <Object.h>
#include <Private_GarbageCollection.h>
#include <String.h>
#include <StringAry.h>
#include <StringLib.h>
#include <Types/DataType.h>

/*
  해야할 것

  JSON에서 사용할 수 있는 String lib 메소드 추가

  TODO String 모듈의 매개변수 이름 통일
*/

int
main(int argc, char const* argv[])
{
  Application_Initialized(argc, argv);
  // Application.ProcessEvent[ProcessEvent_Awake].AddListener(test);
  Application.ApplicationStart();

  String str = String("{ \"type\": \"Test\"}");

  JSONObject obj = JSON_Constructor();
  JSON_Read(obj, str);

  printf("%d", obj == NULL ? 0 : obj->m_FieldLength);

  Application.ApplicationQuit();
  return 0;
}