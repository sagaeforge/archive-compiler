
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_StringAry.h>

#include <stdarg.h>

StringAry
StringAry_Constructor(int pCnt, ...)
{
  StringAry Ary = MemoryCreate(sizeof(StringAry_t));
  if (Ary == NULL) {
    Exception(ERROR,
              "StringAry를 생성하지 못했습니다. [size:%lu]",
              sizeof(StringAry_t));
    return NULL;
  }

  Ary->m_Length = 0;
  Ary->m_Values = NULL;
  if (pCnt == 0)
    return Ary;

  va_list ap;
  va_start(ap, pCnt);
  int i;
  for (i = 0; i < pCnt; i++) {
    String temp = va_arg(ap, String);
    StringAry_Push(Ary, temp);
  }
  va_end(ap);
  return Ary;
}