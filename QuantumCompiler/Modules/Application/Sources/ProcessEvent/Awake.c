
#include "Application.h"
#include "Private_ProcessEvent.h"

#include <stdlib.h>

static void
AddListener(FP_Func Callback)
{
  Events event = &Application.ProcessEvent[ProcessEvent_Awake];

  FuncChainNode* ptr = (FuncChainNode*)malloc(sizeof(FuncChainNode));
  if (ptr == NULL) {
    // TODO Exception 처리
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
RemoveListener(FP_Func Method)
{
  Events event = &Application.ProcessEvent[ProcessEvent_Awake];

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

  // TODO Exception 처리
  // 지정된 함수 포인터가 등록된 함수 포인터가 아닌경우에
}

static void
RemoveAllListener()
{
  Events event = &Application.ProcessEvent[ProcessEvent_Awake];
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
    // TODO Exception 처리
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
  FuncChainNode* Pos = Application.ProcessEvent[ProcessEvent_Awake].m_Nodes;
  while (Pos != NULL) {
    Pos->Callback();
    Pos = Pos->Next;
  }
}
void
ProcessEventModule_Awake_Initialized()
{
  Events event = &Application.ProcessEvent[ProcessEvent_Awake];
  event->AddListener = AddListener;
  event->RemoveListener = RemoveListener;
  event->RemoveAllListener = RemoveAllListener;
  event->Invoke = Invoke;
}