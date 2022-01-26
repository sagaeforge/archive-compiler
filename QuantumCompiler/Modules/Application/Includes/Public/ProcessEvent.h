
#ifndef __PUBLIC_APPLICATION_PROCESSEVENT__
#define __PUBLIC_APPLICATION_PROCESSEVENT__

#include "Types/DataType_ProcessEvent.h"

#include <pthread.h>

void ProcessEventModule_Initialized();

void Update_Wait(pthread_t *Thread);
void Update_AllStart();
void Update_AllStop();
void Update_AllWaitStop();

#endif