
#include <GarbageCollection.h>
#include <Private_String.h>

bool
String_Compare(String Self, String Value)
{
  if (Self->Length != Value->Length)
    return false;

  int i;
  for (i = 0; i < Self->Length; i++)
    if (Self->Value[i] != Value->Value[i])
      return false;
  return true;

  // 모종의 이유로 연결이 안됨 그래서 일단 급한대로 함수를 만들었음.
  // TODO 링킹 옵션 수정
  // return MemoryCompare(Self->Value, Value->Value, Self->Length);
}
