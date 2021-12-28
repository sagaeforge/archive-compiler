
#include "ProgramManager.h"
#include "String.h"
#include <stdio.h>

static void StringModule_StringInit() { printf("Test\n"); }
static void StringModule_StringLibraryInit() { printf("Test\n"); }

void StringModule_Awake() {
  Manager.Init.AddListener(StringModule_StringInit);
  Manager.Init.AddListener(StringModule_StringLibraryInit);
}