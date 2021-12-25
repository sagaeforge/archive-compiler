

#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

void *MemoryCreate(Length Length) {
  // TODO 조건 확인
  void *ptr = malloc(Length);
  if (ptr == NULL)
    Warning("지정된 메모리를 생성할 수 없습니다.");

  GC_Append(ptr, Length);
  return ptr;
}