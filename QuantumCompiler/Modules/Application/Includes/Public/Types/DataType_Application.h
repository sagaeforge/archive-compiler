
#ifndef __PUBLIC_APPLICATION_DATATYPE_APPLICATION__
#define __PUBLIC_APPLICATION_DATATYPE_APPLICATION__

#pragma pack(push, 1)
// clang-format off

#include "DataType.h"
#include "DataType_Exception.h"
#include "DataType_GarbageCollection.h"
#include "DataType_Object.h"
#include "DataType_ProcessEvent.h"

#include <pthread.h>

struct ApplicationManager_t {
  ProcessEvent ProcessEvent[8];

  struct {
    pthread_t         ProcessEvent_UpdateThread;
    pthread_t         ProcessEvent_FixedUpdateThread;
    Length_t          ProcessEvent_FixedUpdateTime;
    bool              ProcessEvent_IsUpdated;
    bool              ProcessEvent_IsFixedUpdated;
    ProcessEventName  ProcessEvent_Status;
    Func_t           ProcessEvent_UpdateStart;
    Func_t           ProcessEvent_UpdateStop;
    Func_t           ProcessEvent_UpdateWaitStop;
    Func_t           ProcessEvent_FixedUpdateStart;
    Func_t           ProcessEvent_FixedUpdateStop;
    Func_t           ProcessEvent_FixedUpdateWaitStop;

    const DataTypeInfo_t* DataTypeTable;

    struct {
      Length_t UsedObjectLength;
      Object   Value[ObjectMaxLength];
      bool     IsUsed[ObjectMaxLength];
    } GarbageCollection_ObjectTable;
    struct
    {
      Length_t TotalUsedMemoryLength;
      Length_t UsedMemoryPageLength;
      MemoryPage_t MemoryPages;
    } GarbageCollection_HeapTable;
  } Member;

  void (*ApplicationInit)     ();
  void (*ApplicationStart)    ();
  void (*ApplicationQuit)     ();

  void (*Update_AllStart)     ();
  void (*Update_AllStop)      ();
  void (*Update_AllWaitStop)  ();
};

// clang-format on
#pragma pack(pop)

#endif