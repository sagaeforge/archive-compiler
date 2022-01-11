
#include "ProgramManager.h"
#include "GarbageCollection.h"
#include "ProcessEvent.h"
#include "String.h"
#include "StringAry.h"

#include <stdlib.h>

struct ProgramManager Application;

static void
ProgramInit()
{
  Application.ProcessEvent[ProcessEvent_Awake].Invoke();
  Application.ProcessEvent[ProcessEvent_Init].Invoke();
  Application.ProcessEvent[ProcessEvent_Start].Invoke();
  Application.Member.ProcessEvent_IsInitialized = true;
}

static void
ProgramStart()
{
  if (!Application.Member.ProcessEvent_IsInitialized)
    ProgramInit();

  Application.ProcessEvent[ProcessEvent_Main].Invoke();
  Application.Member.ProcessEvent_IsStarted = true;
  Application.UpdateStart(ProcessEvent_Update);
  Application.UpdateStart(ProcessEvent_FixedUpdate);
}

static void
ProgramQuit()
{
  Update_AllStop();
  Application.ProcessEvent[ProcessEvent_Quit].Invoke();

  Application.ProcessEvent[ProcessEvent_Awake].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Init].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Start].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Main].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Update].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_FixedUpdate].RemoveAllListener();
  Application.ProcessEvent[ProcessEvent_Quit].RemoveAllListener();

  Length len = Application.Member.GarbageCollection_UsedMemoryPageLength;
  MemoryPage** pages = malloc(sizeof(MemoryPage) * len);
  MemoryPage* page = &Application.Member.GarbageCollection_Pages;
  int i;
  for (i = 0; page != NULL; i++) {
    pages[i] = page;
    int j;
    for (j = 0; j < page->UsedMemoryLength; j++) {
      free(page->Datas[j].Position);
      page->Datas[j].Length = 0;
      page->Datas[j].Policy = MemoryPolicy_None;
      page->Datas[j].Position = NULL;
    }
    page = page->Next;
  }
  for (i = 1; i < len; i++)
    free(pages[i]);
  free(pages);
}

static void
UpdateStart(ProcessEventName Name)
{
  if (Name == ProcessEvent_Update) {
    if (!Application.Member.ProcessEvent_IsUpdated)
      Application.Member.Private_Method.Update_Start();
  } else if (Name == ProcessEvent_FixedUpdate) {
    if (!Application.Member.ProcessEvent_IsFixedUpdated)
      Application.Member.Private_Method.FixedUpdate_Start();
  } else
    // TODO Exception 처리
    // 지정된 프로세스 이벤트 이외의 이벤트를 이 함수에서 사용할 수 없습니다.
    return;
}
static void
UpdateStop(ProcessEventName Name)
{
  if (Name == ProcessEvent_Update) {
    if (Application.Member.ProcessEvent_IsUpdated)
      Application.Member.Private_Method.Update_Stop();
  } else if (Name == ProcessEvent_FixedUpdate) {
    if (Application.Member.ProcessEvent_IsFixedUpdated)
      Application.Member.Private_Method.FixedUpdate_Stop();
  } else
    // TODO Exception 처리
    // 지정된 프로세스 이벤트 이외의 이벤트를 이 함수에서 사용할 수 없습니다.
    return;
}
static void
UpdateStopWait(ProcessEventName Name)
{
  if (Name == ProcessEvent_Update) {
    if (Application.Member.ProcessEvent_IsUpdated)
      Application.Member.Private_Method.Update_StopWait();
  } else if (Name == ProcessEvent_FixedUpdate) {
    if (Application.Member.ProcessEvent_IsFixedUpdated)
      Application.Member.Private_Method.FixedUpdate_StopWait();
  } else
    // TODO Exception 처리
    // 지정된 프로세스 이벤트 이외의 이벤트를 이 함수에서 사용할 수 없습니다.
    return;
}

static bool ProgramManagerModuleInitalized = false;

void
ProgramManager_Init()
{
  ProcessEventModule_Initialized();
  GarbageCollectionModule_Initialized();
  StringModule_Initialized();

  Application.ProgramInit = ProgramInit;
  Application.ProgramStart = ProgramStart;
  Application.ProgramQuit = ProgramQuit;
  Application.UpdateStart = UpdateStart;
  Application.UpdateStop = UpdateStop;
  Application.UpdateStopWait = UpdateStopWait;
  ProgramManagerModuleInitalized = true;
}

void
ProgramManager_ProgramAgumentsSet(int argc, const_chs argv[])
{
  if (!ProgramManagerModuleInitalized)
    // Exception 처리
    return;

  StringAry* temp = StringAry(1, String(argv[0]));
  int i;
  for (i = 1; i < argc; i++)
    StringAryMethod.Push(temp, String(argv[i]));

  Application.ProgramAguments = temp;
}