
#include "ProcessEvent.h"
#include "ProgramManager.h"
#include <stdlib.h>
#include <unistd.h>

typedef ProcessEvent* Events;

static void*
UpdateMethod(void* param)
{
  while (Application.Member.ProcessEvent_IsUpdated) {
    Application.ProcessEvent[ProcessEvent_FixedUpdate].Invoke();
    usleep(((float)1 / (float)Application.Member.ProcessEvent_FixedUpdateTime) *
           1000000);
  }
  return NULL;
}

static void
UpdateStart()
{
  if (Application.ProcessEvent[ProcessEvent_FixedUpdate].Nodes == NULL)
    return;

  Application.Member.ProcessEvent_IsFixedUpdated = true;
  int sig = pthread_create(&Application.Member.ProcessEvent_FixedUpdateThread,
                           NULL,
                           UpdateMethod,
                           NULL);

  if (sig < 0)
    // TODO 오류 처리
    return;
}

static void
UpdateStop()
{
  Application.Member.ProcessEvent_IsFixedUpdated = false;
}
static void
UpdateStopWait()
{
  if (!Application.Member.ProcessEvent_IsUpdated)
    return;

  Application.Member.ProcessEvent_IsUpdated = false;
  Update_Wait(&Application.Member.ProcessEvent_FixedUpdateThread);
}

static void
AddListener(FP_Func Callback)
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];

  FuncChainNode* ptr = (FuncChainNode*)malloc(sizeof(FuncChainNode));
  if (ptr == NULL) {
    // TODO Exception 처리
    return;
  }
  ptr->Next = NULL;
  ptr->Callback = Callback;

  FuncChainNode* Pos = event->Nodes;
  if (Pos == NULL)
    event->Nodes = ptr;
  else {
    while (Pos->Next != NULL)
      Pos = Pos->Next;
    Pos->Next = ptr;
  }

  if (Application.Member.ProcessEvent_IsStarted)
    UpdateStart();
}

static void
RemoveListener(FP_Func Method)
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];

  FuncChainNode* Pos = event->Nodes;
  FuncChainNode* Last = event->Nodes;

  while (Pos != NULL) {
    if (Pos->Callback == Method) {
      if (Pos == event->Nodes)
        event->Nodes = Pos->Next;
      else
        Last->Next = Pos->Next;

      Pos->Callback = NULL;
      Pos->Next = NULL;

      free(Pos);
      break;
    }
    Last = Pos;
    Pos = Pos->Next;
  }

  // TODO Exception 처리
  // 지정된 함수 포인터가 등록된 함수 포인터가 아닌경우에

  if (event->Nodes != NULL)
    UpdateStart();
}

static void
RemoveAllListener()
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];
  int length = 0;
  FuncChainNode* Pos = event->Nodes;

  while (Pos != NULL) {
    length++;
    Pos = Pos->Next;
  }

  if (length == 0)
    return;

  FuncChainNode** Ary = (FuncChainNode**)malloc(sizeof(FuncChainNode) * length);
  if (Ary == NULL) {
    // TODO Exception 처리
    return;
  }

  Pos = event->Nodes;
  int i;
  for (i = 0; i < length; i++, Pos = Pos->Next)
    Ary[i] = Pos;
  for (i = 0; i < length; i++) {
    Ary[i]->Callback = NULL;
    Ary[i]->Next = NULL;
  }
  for (i = 0; i < length; i++)
    free(Ary[i]);
  free(Ary);
  event->Nodes = NULL;
}

static void
Invoke()
{
  FuncChainNode* Pos = Application.ProcessEvent[ProcessEvent_FixedUpdate].Nodes;
  while (Pos != NULL) {
    Pos->Callback();
    Pos = Pos->Next;
  }
}
void
ProcessEventModule_FixedUpdate_Initialized()
{
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];
  event->AddListener = AddListener;
  event->RemoveListener = RemoveListener;
  event->RemoveAllListener = RemoveAllListener;
  event->Invoke = Invoke;

  Application.Member.Private_Method.FixedUpdate_Start = UpdateStart;
  Application.Member.Private_Method.FixedUpdate_Stop = UpdateStop;
  Application.Member.Private_Method.FixedUpdate_StopWait = UpdateStopWait;
}