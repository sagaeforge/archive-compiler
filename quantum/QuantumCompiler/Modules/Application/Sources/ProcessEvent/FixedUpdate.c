
#include <Application.h>
#include <Exception.h>
#include <Private_ProcessEvent.h>

#include <stdlib.h>
#include <unistd.h>

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
  if (Application.ProcessEvent[ProcessEvent_FixedUpdate].m_Nodes == NULL)
    return;

  Application.Member.ProcessEvent_IsFixedUpdated = true;
  int sig = pthread_create(&Application.Member.ProcessEvent_FixedUpdateThread,
                           NULL,
                           UpdateMethod,
                           NULL);

  if (sig < 0) {
    Exception(ERROR, "업데이트용 쓰레드를 생성하지 못했습니다.");
    return;
  }
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
AddListener(Func_t Callback)
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];

  FuncChainNode* ptr = (FuncChainNode*)malloc(sizeof(FuncChainNode));
  if (ptr == NULL) {
    Exception(ERROR,
              "FixedUpdate 함수 노드를 생성하지 못했습니다. [size:%lu]",
              sizeof(FuncChainNode));
    return;
  }
  ptr->Next = NULL;
  ptr->Callback = Callback;

  FuncChainNode* Pos = event->m_Nodes;
  if (Pos == NULL)
    event->m_Nodes = ptr;
  else {
    while (Pos->Next != NULL)
      Pos = Pos->Next;
    Pos->Next = ptr;
  }

  if (Application.Member.ProcessEvent_Status == ProcessEvent_Main)
    UpdateStart();
}

static void
RemoveListener(Func_t Method)
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];

  FuncChainNode* Pos = event->m_Nodes;
  FuncChainNode* Last = event->m_Nodes;

  while (Pos != NULL) {
    if (Pos->Callback == Method) {
      if (Pos == event->m_Nodes)
        event->m_Nodes = Pos->Next;
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

  if (event->m_Nodes != NULL)
    UpdateStart();

  Exception(ERROR, "할당한 FixedUpdate 함수가 아닙니다. [func:%p]", Method);
}

static void
RemoveAllListener()
{
  UpdateStopWait();
  Events event = &Application.ProcessEvent[ProcessEvent_FixedUpdate];
  int length = 0;
  FuncChainNode* Pos = event->m_Nodes;

  while (Pos != NULL) {
    length++;
    Pos = Pos->Next;
  }

  if (length == 0)
    return;

  FuncChainNode** Ary = (FuncChainNode**)malloc(sizeof(FuncChainNode) * length);
  if (Ary == NULL) {
    Exception(ERROR,
              "임시 객체를 생성하지 못했습니다. [size:%lu]",
              sizeof(FuncChainNode) * length);
    return;
  }

  Pos = event->m_Nodes;
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
  event->m_Nodes = NULL;
}

static void
Invoke()
{
  FuncChainNode* Pos =
    Application.ProcessEvent[ProcessEvent_FixedUpdate].m_Nodes;
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

  Application.Member.ProcessEvent_FixedUpdateStart = UpdateStart;
  Application.Member.ProcessEvent_FixedUpdateStop = UpdateStop;
  Application.Member.ProcessEvent_FixedUpdateWaitStop = UpdateStopWait;
}