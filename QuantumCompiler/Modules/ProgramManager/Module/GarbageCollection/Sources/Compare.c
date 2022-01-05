
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

bool MemoryCompare(void *Obj1, void *Obj2, Length Length) {
  char *a = (char *)Obj1;
  char *b = (char *)Obj2;
  int i = 0;
  while (i < Length) {
    if (*a != *b)
      return false;
    a++, b++, i++;
  }
  return true;
}