
#include "ProgramManager.h"
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

struct ProgramManager Manager;

#define Generator_Chain_AddListener(name)                                      \
  static void name##_AddListener(FP_Func Method) {                             \
    FuncChainNode *ptr = (FuncChainNode *)malloc(sizeof(FuncChainNode));       \
    if (ptr == NULL) {                                                         \
      printf("ERR > NODE 생성 실패");                                          \
      return;                                                                  \
    }                                                                          \
    ptr->Next = NULL;                                                          \
    FuncChainNode *Pos = Manager.name.Nodes;                                   \
    if (Pos == NULL)                                                           \
      Manager.name.Nodes = Pos = ptr;                                          \
    else {                                                                     \
      while (Pos->Next != NULL)                                                \
        Pos = Pos->Next;                                                       \
      Pos->Next = ptr;                                                         \
      Pos = Pos->Next;                                                         \
    }                                                                          \
    Pos->Method = Method;                                                      \
  }
#define Generator_Chain_RemoveListener(name)                                   \
  static void name##_RemoveListener(FP_Func Method) {                          \
    FuncChainNode *Pos = Manager.name.Nodes;                                   \
    FuncChainNode *Last = Manager.name.Nodes;                                  \
    while (Pos != NULL) {                                                      \
      if (Pos->Method == Method) {                                             \
        if (Pos == Manager.name.Nodes) {                                       \
          Manager.name.Nodes = Pos->Next;                                      \
        } else {                                                               \
          Last->Next = Pos->Next;                                              \
        }                                                                      \
        Pos->Method = NULL;                                                    \
        Pos->Next = NULL;                                                      \
        free(Pos);                                                             \
        break;                                                                 \
      }                                                                        \
      Last = Pos;                                                              \
      Pos = Pos->Next;                                                         \
    }                                                                          \
  }
#define Generator_Chain_RemoveAllListener(name)                                \
  static void name##_RemoveAllListener() {                                     \
    int length = 1;                                                            \
    FuncChainNode *Pos = Manager.name.Nodes;                                   \
    if (Pos == NULL)                                                           \
      return;                                                                  \
    while (Pos->Next != NULL) {                                                \
      length++;                                                                \
      Pos = Pos->Next;                                                         \
    }                                                                          \
    void *tempAry = malloc(sizeof(FuncChainNode) * length);                    \
    if (tempAry == NULL) {                                                     \
      Error("버퍼공간을 확보하지 못했습니다.");                                \
    }                                                                          \
    FuncChainNode **Ary = (FuncChainNode **)tempAry;                           \
    Pos = Manager.name.Nodes;                                                  \
    int i = 0;                                                                 \
    while (Pos != NULL) {                                                      \
      Ary[i++] = Pos;                                                          \
      Pos = Pos->Next;                                                         \
    }                                                                          \
    for (i = 0; i < length; i++)                                               \
      free(Ary[i]);                                                            \
    free(tempAry);                                                             \
  }
#define Generator_Chain_Invoke(name)                                           \
  static void name##_Invoke() {                                                \
    FuncChainNode *Pos = Manager.name.Nodes;                                   \
    while (Pos != NULL) {                                                      \
      Pos->Method();                                                           \
      Pos = Pos->Next;                                                         \
    }                                                                          \
  }

Generator_Chain_AddListener(Awake);
Generator_Chain_RemoveListener(Awake);
Generator_Chain_RemoveAllListener(Awake);
Generator_Chain_Invoke(Awake);

Generator_Chain_AddListener(Init);
Generator_Chain_RemoveListener(Init);
Generator_Chain_RemoveAllListener(Init);
Generator_Chain_Invoke(Init);

Generator_Chain_AddListener(Start);
Generator_Chain_RemoveListener(Start);
Generator_Chain_RemoveAllListener(Start);
Generator_Chain_Invoke(Start);

Generator_Chain_AddListener(Quit);
Generator_Chain_RemoveListener(Quit);
Generator_Chain_RemoveAllListener(Quit);
Generator_Chain_Invoke(Quit);

static void ProgramManager_ProgramQuit() {
  Manager.Quit.Invoke();

  Manager.Awake.RemoveAllListener();
  Manager.Init.RemoveAllListener();
  Manager.Start.RemoveAllListener();
  Manager.Quit.RemoveAllListener();

  Manager.GarbageCollection.Method.Clear();
  int i = 0;
  MemoryPage *page = Manager.GarbageCollection.Pages.Next;
  if (Manager.GarbageCollection.UsedMemoryPageLength != 1) {
    MemoryPage **Temp =
        malloc(sizeof(MemoryPage) *
               (Manager.GarbageCollection.UsedMemoryPageLength - 1));

    while (page != NULL) {
      Temp[i++] = page;
      page = page->Next;
    }
    for (i = 0; i < Manager.GarbageCollection.UsedMemoryPageLength - 1; i++)
      free(Temp[i]);
    free(Temp);
  }

  exit(0);
}

static void PrograManager_ProgramStart() {
  Manager.Awake.Invoke();
  Manager.Init.Invoke();
  Manager.Start.Invoke();
}

void ProgramManager_Init() {
  // clang-format off
  Manager.Awake.AddListener       = Awake_AddListener;
  Manager.Awake.RemoveListener    = Awake_RemoveListener;
  Manager.Awake.RemoveAllListener = Awake_RemoveAllListener;
  Manager.Awake.Invoke            = Awake_Invoke;

  Manager.Init.AddListener        = Init_AddListener;
  Manager.Init.RemoveListener     = Init_RemoveListener;
  Manager.Init.RemoveAllListener  = Init_RemoveAllListener;
  Manager.Init.Invoke             = Init_Invoke;

  Manager.Start.AddListener       = Start_AddListener;
  Manager.Start.RemoveListener    = Start_RemoveListener;
  Manager.Start.RemoveAllListener = Start_RemoveAllListener;
  Manager.Start.Invoke            = Start_Invoke;
  
  Manager.Quit.AddListener        = Quit_AddListener;
  Manager.Quit.RemoveListener     = Quit_RemoveListener;
  Manager.Quit.RemoveAllListener  = Quit_RemoveAllListener;
  Manager.Quit.Invoke             = Quit_Invoke;
  // clang-format on

  Manager.GarbageCollection.Method.MemoryCreate = MemoryCreate;
  Manager.GarbageCollection.Method.MemoryConstCreate = MemoryConstCreate;
  Manager.GarbageCollection.Method.MemoryRemove = MemoryRemove;
  Manager.GarbageCollection.Method.MemoryCompare = MemoryCompare;
  Manager.GarbageCollection.Method.MemorySet = MemorySet;
  Manager.GarbageCollection.Method.MemoryCopy = MemoryCopy;
  Manager.GarbageCollection.Method.MemoryLength = MemoryLength;
  Manager.GarbageCollection.Method.MemoryMove = MemoryMove;
  Manager.GarbageCollection.Method.MemorySwap = MemorySwap;

  Manager.GarbageCollection.Method.Clear = Clear;
  Manager.GarbageCollection.Method.Info = Info;
  Manager.GarbageCollection.Method.Memory = Memory;

  Manager.GarbageCollection.Method.Policy = Policey;
  Manager.GarbageCollection.Method.PolicyAppend = Policey_Append;
  Manager.GarbageCollection.Method.PolicyRemove = Policey_Remove;

  Manager.GarbageCollection.UsedMemoryLength = 0;
  Manager.GarbageCollection.UsedMemoryPageLength = 1;

  Manager.Method.ProgramStart = PrograManager_ProgramStart;
  Manager.Method.ProgramQuit = ProgramManager_ProgramQuit;
}
