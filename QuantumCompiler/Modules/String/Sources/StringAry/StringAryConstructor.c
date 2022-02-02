
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_StringAry.h>

#include <stdarg.h>

StringAry
StringAryConstructor(int Cnt, ...)
{
  StringAry Ary = MemoryCreate(sizeof(StringAry_t));
  if (Ary == NULL) {
    Exception(ERROR,
              "StringAry를 생성하지 못했습니다. [size:%lu]",
              sizeof(StringAry_t));
    return NULL;
  }

  Ary->Length = 0;
  Ary->Values = NULL;
  if (Cnt == 0)
    return Ary;

  va_list ap;
  va_start(ap, Cnt);
  int i;
  for (i = 0; i < Cnt; i++) {
    String temp = va_arg(ap, String);
    StringAry_Push(Ary, temp);
  }
  va_end(ap);
  return Ary;
}