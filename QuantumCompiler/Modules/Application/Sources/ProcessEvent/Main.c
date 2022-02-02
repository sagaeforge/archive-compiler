
#include <Application.h>
#include <Exception.h>
#include <Private_ProcessEvent.h>

#include <stdlib.h>

static void
AddListener(Func_t Callback)
{
  Events event = &Application.ProcessEvent[ProcessEvent_Main];

  FuncChainNode* ptr = (FuncChainNode*)malloc(sizeof(FuncChainNode));
  if (ptr == NULL) {
    Exception(ERROR,
              "Main 함수 노드를 생성하지 못했습니다. [size:%lu]",
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
}

static void
RemoveListener(Func_t Method)
{
  Events event = &Application.ProcessEvent[ProcessEvent_Main];

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

  Exception(ERROR, "할당한 Main 함수가 아닙니다. [func:%p]", Method);
}

static void
RemoveAllListener()
{
  Events event = &Application.ProcessEvent[ProcessEvent_Main];
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
  FuncChainNode* Pos = Application.ProcessEvent[ProcessEvent_Main].m_Nodes;
  while (Pos != NULL) {
    Pos->Callback();
    Pos = Pos->Next;
  }
}
void
ProcessEventModule_Main_Initialized()
{
  Events event = &Application.ProcessEvent[ProcessEvent_Main];
  event->AddListener = AddListener;
  event->RemoveListener = RemoveListener;
  event->RemoveAllListener = RemoveAllListener;
  event->Invoke = Invoke;
}