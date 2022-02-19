
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nDigitAry.h>

nDigitAry_ptr
DigitAry_Constructor()
{
  nDigitAry_ptr _Ary = (nDigitAry_ptr)malloc(sizeof(nDigitAry_t));
  if (!_Ary) {
    Exception(ERROR, "정수 배열을 생성할 수 없습니다.");
    return NULL;
  }

  _Ary->m_Length = 0;
  _Ary->m_Nodes = NULL;
  return _Ary;
}
