
#ifndef __PRIVATE_APPLICATION_PROCESSEVENT__
#define __PRIVATE_APPLICATION_PROCESSEVENT__

#include <ProcessEvent.h>

typedef ProcessEvent* Events;

void
ProcessEventModule_Awake_Initialized();
void
ProcessEventModule_Init_Initialized();
void
ProcessEventModule_Start_Initialized();
void
ProcessEventModule_Main_Initialized();
void
ProcessEventModule_Quit_Initialized();
void
ProcessEventModule_Update_Initialized();
void
ProcessEventModule_FixedUpdate_Initialized();

#endif