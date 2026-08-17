
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nStringAry.h>

nStringAry_t*
StringAry_toAry(const nStringAry_t* pSelf)
{
  nStringAry_ptr _Ary = nStringAry(0);
  nString_ptr* _Nodes = (nString_ptr*)malloc(sizeof(nString_ptr) * pSelf->m_Length);
  Index_t _i;
  if (!_Nodes) {
    Exception(ERROR, "nStringAry을 생성할 수 없습니다.");
    free(_Ary);
    return NULL;
  }

  for (_i = 0; _i < pSelf->m_Length; _i++)
    _Nodes[_i] = StringAry_get(pSelf, _i);

  _Ary->m_Lists = NULL;
  _Ary->m_Arys = _Nodes;
  _Ary->m_Size = pSelf->m_Length;
  _Ary->m_Length = pSelf->m_Length;
  _Ary->m_AryType = k_nStringAry_Ary;

  return _Ary;
}
