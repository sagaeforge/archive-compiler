
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nStringAry.h>

nStringAry_t*
StringAry_Constructor(const Length_t pCount, ...)
{
  nStringAry_ptr _Ary = (nStringAry_ptr)malloc(sizeof(nStringAry_t));
  if (!_Ary) {
    Exception(ERROR, "nStringAry을 생성할 수 없습니다.");
    return NULL;
  }

  if (pCount == 0) {
    _Ary->m_Length = 0;
    _Ary->m_AryType = k_nStringAry_List;
    _Ary->m_Arys = NULL;
  } else {
    _Ary->m_Size = pCount;
    _Ary->m_Length = pCount;
    _Ary->m_AryType = k_nStringAry_Ary;

    nString_ptr* _Nodes = (nString_ptr*)malloc(sizeof(nString_ptr) * pCount);
    if (!_Nodes) {
      Exception(ERROR, "nStringAry을 생성할 수 없습니다.");
      free(_Ary);
      return NULL;
    }

    va_list _lists;
    va_start(_lists, pCount);
    Index_t _i;
    for (_i = 0; _i < pCount; _i++) {
      nString_ptr _node = va_arg(_lists, nString_ptr);
      _Nodes[_i] = _node;
    }
    _Ary->m_Arys = _Nodes;
    va_end(_lists);
  }

  return _Ary;
}
