
#include "Application.h"
#include "Private_ProcessEvent.h"

#include <pthread.h>

void ProcessEventModule_Initialized() {
  ProcessEventModule_Awake_Initialized();
  ProcessEventModule_Init_Initialized();
  ProcessEventModule_Start_Initialized();
  ProcessEventModule_Main_Initialized();
  ProcessEventModule_Quit_Initialized();
  ProcessEventModule_Update_Initialized();
  ProcessEventModule_FixedUpdate_Initialized();

  Application.Member.ProcessEvent_FixedUpdateTime = 60;
  Application.Member.ProcessEvent_IsUpdated = false;
  Application.Member.ProcessEvent_IsFixedUpdated = false;
}

void Update_Wait(pthread_t *Thread) {
  int status;
  pthread_join(*Thread, (void **)&status);
}

void Update_AllStart() {
  if (!Application.Member.ProcessEvent_IsUpdated) {
    Application.Member.ProcessEvent_IsUpdated = true;
    Application.Member.ProcessEvent_UpdateStart();
  }

  if (!Application.Member.ProcessEvent_IsFixedUpdated) {
    Application.Member.ProcessEvent_IsFixedUpdated = true;
    Application.Member.ProcessEvent_FixedUpdateStart();
  }
}

void Update_AllStop() {
  if (Application.Member.ProcessEvent_IsUpdated) {
    Application.Member.ProcessEvent_IsUpdated = false;
    Application.Member.ProcessEvent_UpdateStop();
  }

  if (Application.Member.ProcessEvent_IsFixedUpdated) {
    Application.Member.ProcessEvent_IsFixedUpdated = false;
    Application.Member.ProcessEvent_FixedUpdateStop();
  }
}

void Update_AllWaitStop() {
  if (Application.Member.ProcessEvent_IsUpdated) {
    Application.Member.ProcessEvent_IsUpdated = false;
    Application.Member.ProcessEvent_UpdateWaitStop();
  }

  if (Application.Member.ProcessEvent_IsFixedUpdated) {
    Application.Member.ProcessEvent_IsFixedUpdated = false;
    Application.Member.ProcessEvent_FixedUpdateWaitStop();
  }
}