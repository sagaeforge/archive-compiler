
#include <Application.h>
#include <Exception.h>
#include <Object.h>
#include <Private_GarbageCollection.h>
#include <Private_ProcessEvent.h>
#include <String.h>
#include <StringAry.h>

#include <stdlib.h>

struct ApplicationManager_t Application;

static void
Application_Init()
{
  Application.ProcessEvent[ProcessEvent_Awake].Invoke();
  Application.ProcessEvent[ProcessEvent_Init].Invoke();
  Application.ProcessEvent[ProcessEvent_Start].Invoke();

  Application.Member.ProcessEvent_Status = ProcessEvent_Start;
}
static void
Application_Start()
{
  if (Application.Member.ProcessEvent_Status != ProcessEvent_Main)
    Application_Init();

  Application.ProcessEvent[ProcessEvent_Main].Invoke();
  Application.Member.ProcessEvent_Status = ProcessEvent_Main;
}
static void
Application_Quit()
{
  if (Application.Member.ProcessEvent_ProgramQuit)
    return;

  Application.ProcessEvent[ProcessEvent_Quit].Invoke();
  Update_AllWaitStop();

  Application.ProcessEvent[ProcessEvent_Awake].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Init].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Start].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Main].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Update].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_FixedUpdate].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Quit].RemoveAllListener();

  // GarbageCollection 해제
  int i, j;
  for (i = 0; i < ObjectMaxLength; i++)
    free(Application.Member.GarbageCollection_ObjectTable.Value[i]);

  MemoryPage page = &Application.Member.GarbageCollection_HeapTable.MemoryPages;
  MemoryPage* PageAry = (MemoryPage*)malloc(
    sizeof(MemoryPage_t) *
    Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength);
  if (PageAry == NULL)
    Exception(
      WARNING,
      "임시 객체를 생성하지 못했습니다. [size:%lu]",
      sizeof(MemoryPage_t) *
        Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength);
  i = 0;
  while (page != NULL) {
    // 페이지를 등록함
    if (page != &Application.Member.GarbageCollection_HeapTable.MemoryPages)
      PageAry[i++] = page;
    // 원소를 삭제함
    for (j = 0; j < MemoryMaxLength; j++) {
      if (page->Nodes[j].m_Value == NULL)
        break;
      free(page->Nodes[j].m_Value);
    }
    page = page->Next;
  }
  // 생성함 페이지도 삭제함.
  for (j = 0; j < i; j++)
    free(PageAry[i]);
  free(PageAry);

  Application.Member.ProcessEvent_ProgramQuit = true;
}

void
Application_Initialized(int argc, char const* argv[])
{
  ProcessEventModule_Initialized();
  GarbageCollectionModule_Initialized();
  StringModule_Initialized();

  Application.ApplicationInit = Application_Init;
  Application.ApplicationStart = Application_Start;
  Application.ApplicationQuit = Application_Quit;

  Application.Update_AllStart = Update_AllStart;
  Application.Update_AllStop = Update_AllStop;
  Application.Update_AllWaitStop = Update_AllWaitStop;
  Application.Member.DataTypeTable = g_DataTypeTable;

  int i;
  StringAry Param = StringAry_Constructor(0);
  for (i = 0; i < argc; i++) {
    StringAryMethod.Push(Param, String(argv[i]));
  }
  *(StringAry*)(&Application.ProgramParam) = Param;
}