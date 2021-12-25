
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

static void Swap(void *ptr1, void *ptr2, Length length) {
  char *a = (char *)ptr1;
  char *b = (char *)ptr2;

  int i = 0;
  char t = '\0';
  while (i < length) {
    t = *a;
    *a = *b;
    *b = t;
    i++, a++, b++;
  }
}
void MemorySwap(void *Src, void *Data, Length Length) {
  if (GC_CreateCheck(Src, Data))
    return;
  if (Policey(Src, MemoryPolicey_NotMemorySwap) ||
      Policey(Data, MemoryPolicey_NotMemorySwap))
    return;
  if (Policey(Src, MemoryPolicey_Const))
    return;
  if (GC_IndexOfExceptionCheck(Src, Length) ||
      GC_IndexOfExceptionCheck(Data, Length))
    return;

  Swap(Src, Data, Length);
}