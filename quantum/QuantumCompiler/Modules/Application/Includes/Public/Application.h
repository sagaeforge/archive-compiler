
#ifndef __PUBLIC_APPLICATION_APPLICATION__
#define __PUBLIC_APPLICATION_APPLICATION__

#include <Types/DataType_Application.h>

extern struct ApplicationManager_t Application;

void
Application_Initialized(int argc, char const* argv[]);

#endif