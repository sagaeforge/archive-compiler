
#include "Private_String.h"
#include "Private_StringAry.h"
#include "Private_StringLib.h"

// TODO 오류 테스트

StringAry*
String_Split(String* Self, String* Value)
{
  StringAry* Ary = StringAryConstructor(0);
  Length_t Cnt = String_Count(Self, Value);

  if (Cnt == 0) {
    StringAry_Push(Ary, Self);
    return Ary;
  }

  Index_t Start = 0;
  int i;
  for (i = 0; i < Cnt; i++) {
    Index_t index = String_IndexFor(Self, Value, i);
    StringAry_Push(Ary, String_Extract(Self, Start, index));
    Start = index + Value->Length;
  }

  Index_t index = String_IndexFor(Self, Value, i - 1) + Value->Length;
  StringAry_Push(Ary, String_Extract(Self, index, Self->Length));

  return Ary;
}
