
#include "Application.h"
#include "Object.h"
#include "Private_GarbageCollection.h"
#include "Private_ProcessEvent.h"

#include <stdlib.h>

// clang-format off
// const라 접근하기 애매한것들 초기화

struct ApplicationManager_t Application;
// clang-format on

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
  MemoryPage* PageAry = (MemoryPage*)malloc(sizeof(MemoryPage_t));
  if (PageAry == NULL)
    // TODO Exception 처리
    return;
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
}

void
Application_Initialized()
{
  ProcessEventModule_Initialized();
  GarbageCollectionModule_Initialized();

  Application.ApplicationInit = Application_Init;
  Application.ApplicationStart = Application_Start;
  Application.ApplicationQuit = Application_Quit;

  Application.Update_AllStart = Update_AllStart;
  Application.Update_AllStop = Update_AllStop;
  Application.Update_AllWaitStop = Update_AllWaitStop;
  Application.Member.DataTypeTable = g_DataTypeTable;
}
