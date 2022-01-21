
#include "Application.h"
#include "Object.h"
#include "Private_GarbageCollection.h"
#include "Private_ProcessEvent.h"

#include <stdlib.h>

// clang-format off
// const라 접근하기 애매한것들 초기화

struct ApplicationManager_t Application = {
  .Member.SystemDataTypeTable = g_SystemDataTypeTable,
  .Member.CustumDataTypeTable = g_CustumDataTypeTable,
};
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
  int i;
  for (i = 0; i < ObjectMaxLength; i++)
    free(Application.Member.GarbageCollection_ObjectTable.Value[i]);
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
}
