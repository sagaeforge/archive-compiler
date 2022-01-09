
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

void
MemorySet(void* Src, int value, Length WordSize, Length Length)
{
  int i = 0, j = 0;
  Length *= WordSize;

  char* a = (char*)Src;
  char* b = (char*)&value;
  const char* backup = b;
  while (i < Length) {
    while (j < WordSize) {
      *a = *b;
      a++, b++, i++, j++;
    }
    j = 0;
    b = (char*)backup;
  }
}
