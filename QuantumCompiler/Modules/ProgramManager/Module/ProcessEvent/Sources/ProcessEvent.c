
#include "ProcessEvent.h"
#include "Private_ProcessEvent.h"
#include "ProgramManager.h"

void
ProcessEventModule_Initialized()
{
  ProcessEventModule_Awake_Initialized();
  ProcessEventModule_Init_Initialized();
  ProcessEventModule_Start_Initialized();
  ProcessEventModule_Main_Initialized();
  ProcessEventModule_Update_Initialized();
  ProcessEventModule_FixedUpdate_Initialized();
  ProcessEventModule_Quit_Initialized();

  Application.Member.ProcessEvent_IsInitialized = false;
  Application.Member.ProcessEvent_IsStarted = false;
  Application.Member.ProcessEvent_IsUpdated = false;
  Application.Member.ProcessEvent_IsFixedUpdated = false;
  Application.Member.ProcessEvent_FixedUpdateTime = 60;
}

void
Update_Wait(pthread_t* Thread)
{
  int status;
  pthread_join(*Thread, (void**)&status);
}
void
Update_AllStop()
{
  if (Application.Member.ProcessEvent_IsUpdated) {
    Application.Member.ProcessEvent_IsUpdated = false;
    Update_Wait(&Application.Member.ProcessEvent_UpdateThread);
  }
  if (Application.Member.ProcessEvent_IsFixedUpdated) {
    Application.Member.ProcessEvent_IsFixedUpdated = false;
    Update_Wait(&Application.Member.ProcessEvent_FixedUpdateThread);
  }
}