
#include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

#include <Application.h>
// #include <Exception.h>
// #include <GarbageCollection.h>
#include <Json.h>
#include <Object.h>
// #include <Private_GarbageCollection.h>
#include <String.h>
// #include <StringAry.h>
// #include <StringLib.h>
#include <Types/DataType.h>

/*
  해야할 것

  JSON에서 사용할 수 있는 String lib 메소드 추가
*/

int
main(int argc, char const* argv[])
{
  Application_Initialized(argc, argv);
  // Application.ProcessEvent[ProcessEvent_Awake].AddListener(test);
  Application.ApplicationStart();

  // String str = String("{ \"type\": \"Test\"}");
  FILE* fp = fopen("./test.json", "r+");
  if (fp == NULL)
    return -1;

  Object ob = Object(56);
  int te = UnBoxing(int)(ob);

  // JSONObject obj = JSON_Constructor();
  // JSON_Read(obj, fp);
  // JSONAry obj2 = obj->m_Nodes->m_Value.ReferenceValue;

  // printf("%d", obj == NULL ? 0 : obj->m_FieldLength);

  fclose(fp);

  Application.ApplicationQuit();
  return 0;
}