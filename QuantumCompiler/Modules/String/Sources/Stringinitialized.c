
#include "ProgramManager.h"
#include "String.h"

static void StringModule_StringInit() {}
static void StringModule_StringLibraryInit() {}

void StringModule_Awake() {
  Manager.Init.AddListener(StringModule_StringInit);
  Manager.Init.AddListener(StringModule_StringLibraryInit);
}