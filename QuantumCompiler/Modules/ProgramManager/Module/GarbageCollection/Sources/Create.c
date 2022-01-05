

#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

void *MemoryCreate(Length Length) {
  void *ptr = malloc(Length);
  if (ptr == NULL)
    // TODO Exception 처리
    return NULL;

  GC_Append(ptr, Length);
  return ptr;
}