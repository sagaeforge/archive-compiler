
#ifndef __PUBLIC_PROCESSEVENT__
#define __PUBLIC_PROCESSEVENT__

#include "Types/DataTypes_ProcessEvent.h"

void ProcessEventModule_Initialized();
void Update_Wait(pthread_t *Thread);
void Update_AllStop();
#endif