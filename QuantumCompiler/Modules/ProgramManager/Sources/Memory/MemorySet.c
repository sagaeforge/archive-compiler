
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

void MemorySet(void *Src, int value, Length WordSize, Length Length) {
  int i = 0, j = 0;
  Length *= WordSize;
  MemoryInfo info = Info(Src);
  // clang-format off
  bool IsChecked[] = { 
    !info.IsFounded,
    Policey(Src, MemoryPolicey_NotMemorySet),
    Policey(Src, MemoryPolicey_Const),
    GC_IndexOfExceptionCheck(Src, Length),
    (WordSize == 0 || WordSize == 3 || WordSize > 4)
  };
  for (i = 0; i < 5; i++)
    if(IsChecked[i]) {
      switch (i) {
        case 0: Warning("GC에서 할당한 메모리가 아닙니다. (%p)", Src); break;
        case 1: Warning("%p는 MemorySet을 사용할 수 없습니다.", Src); break;
        case 2: Warning("%p의 메모리를 수정할 수 없습니다.", Src); break;
        case 3: Warning("GC에서 할당한 메모리 크기(%u)보다 큽니다. (Length: %u)", info.Length, Length); break;
        case 4: Warning("워드 크기가 \"1, 2, 4\"가 아닙니다. (WordSize: %u)", WordSize); break;
      }
      return;
    }
  // clang-format on

  char *a = (char *)Src;
  char *b = (char *)&value;
  const char *backup = b;
  while (i < Length) {
    while (j < WordSize) {
      *a = *b;
      a++, b++, i++, j++;
    }
    j = 0;
    b = (char *)backup;
  }
}
