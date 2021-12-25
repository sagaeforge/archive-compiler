
#include "Delegate.h"
#include "Exception.h"
#include <stdio.h>
#include <stdlib.h>

static void AddListener(struct __FuncChain *Chain, FP_Func Method) {
  FuncChainNode *ptr = (FuncChainNode *)malloc(sizeof(FuncChainNode));
  if (ptr == NULL) {
    Warning("델리게이트 노드 생성 실패");
    return;
  }
  ptr->Next = NULL;
  FuncChainNode *Pos = Chain->Nodes;
  if (Pos == NULL)
    Chain->Nodes = Pos = ptr;
  else {
    while (Pos->Next != NULL)
      Pos = Pos->Next;
    Pos->Next = ptr;
    Pos = Pos->Next;
  }
  Pos->Method = Method;
}
static void RemoveListener(struct __FuncChain *Chain, FP_Func Method) {
  FuncChainNode *Pos = Chain->Nodes;
  FuncChainNode *Last = Chain->Nodes;
  while (Pos != NULL) {
    if (Pos->Method == Method) {
      if (Pos == Chain->Nodes) {
        Chain->Nodes = Pos->Next;
      } else {
        Last->Next = Pos->Next;
      }
      Pos->Method = NULL;
      Pos->Next = NULL;
      free(Pos);
      break;
    }
    Last = Pos;
    Pos = Pos->Next;
  }
}
static void RemoveAllListener(struct __FuncChain *Chain) {
  int length = 1;
  FuncChainNode *Pos = Chain->Nodes;
  if (Pos == NULL)
    return;
  while (Pos->Next != NULL) {
    length++;
    Pos = Pos->Next;
  }
  void *tempAry = malloc(sizeof(FuncChainNode) * length);
  if (tempAry == NULL) {
    Error("버퍼공간을 확보하지 못했습니다.");
  }
  FuncChainNode **Ary = (FuncChainNode **)tempAry;
  Pos = Chain->Nodes;
  int i = 0;
  while (Pos != NULL) {
    Ary[i++] = Pos;
    Pos = Pos->Next;
  }
  for (i = 0; i < length; i++)
    free(Ary[i]);
  free(tempAry);
}
static void Invoke(struct __FuncChain *Chain) {
  FuncChainNode *Pos = Chain->Nodes;
  while (Pos != NULL) {
    Pos->Method();
    Pos = Pos->Next;
  }
}

void FuncChain_Setting(FuncChain *Chain) {
  Chain->AddListener = AddListener;
  Chain->RemoveListener = RemoveListener;
  Chain->RemoveAllListener = RemoveAllListener;
  Chain->Invoke = Invoke;
}