
#include "Chs.h"
#include "GarbageCollection.h"

Length __Chslen(const_chs Value) {
  if (Value == NULL)
    return 0;

  Index i = 0;
  while (Value[i] != '\0')
    i++;
  return i;
}
Length __Wcslen(const_wcs Value) {
  if (Value == NULL)
    return 0;

  Index i = 0;
  while (Value[i] != L'\0')
    i++;
  return i;
}
wcs __WcsCreate(Length Length) {
  wcs temp = MemoryCreate(sizeof(wchar_t) * (Length + 1));
  if (temp == NULL) {
    Warning("메모리를 생성할 수 없습니다. (Size:%lu)",
            sizeof(wchar_t) * (Length + 1));
    return NULL;
  }
  MemorySet(temp, 0, sizeof(wchar_t), (Length + 1));
  return temp;
}
void __WcsChsSet(wcs Obj1, const_chs Obj2, Length Length) {
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
  Obj1[i] = L'\0';
}
void __WcsWcsSet(wcs Obj1, const_wcs Obj2, Length Length) {
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
  Obj1[i] = L'\0';
}