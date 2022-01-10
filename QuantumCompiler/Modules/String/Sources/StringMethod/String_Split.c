
#include "Private_StringAry.h"
#include "Private_String.h"
#include "Private_StringLib.h"
#include "ProgramManager.h"

// 오류 테스트 나중에 구현

StringAry*
String_Split(String* Self, String* Value)
{
  StringAry *Ary = StringAryConstructor(0);
  Length Cnt = String_Count(Self, Value);
  
  if(Cnt == 0)
  {
    StringAry_Push(Ary, Self);
    return Ary;
  }
  
  Index Start = 0;
  int i;
  for (i = 0; i < Cnt; i++)
  {
    Index index = String_IndexFor(Self, Value, i);
    StringAry_Push(
      Ary,
      String_Extract(Self, Start, index)
    )
    Start = index + Value->Length;
  }
  
  return Ary;
}
