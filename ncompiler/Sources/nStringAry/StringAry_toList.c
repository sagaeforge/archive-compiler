
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nStringAry.h>

nStringAry_t*
StringAry_toList(const nStringAry_t* pSelf)
{
  nStringAry_ptr _Ary = (nStringAry_ptr)malloc(sizeof(nStringAry_t));
  if (!_Ary) {
    Exception(ERROR, "nStringAry을 생성할 수 없습니다.");
    return NULL;
  }

  nStringAryNode_ptr* _ary = (nStringAryNode_ptr*)malloc(sizeof(nStringAryNode_ptr) * pSelf->m_Length);
  if (!_ary) {
    Exception(ERROR, "nStringAry을 생성할 수 없습니다.");
    return NULL;
  }

  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++) {
    _ary[_i] = StringAry_MakeNode();
    _ary[_i]->m_Value = StringAry_get(pSelf, _i);
  }
  for (_i = 0; _i < pSelf->m_Length - 1; _i++)
    _ary[_i]->Next = _ary[_i + 1];

  _Ary->m_AryType = k_nStringAry_List;
  _Ary->m_Length = pSelf->m_Length;
  _Ary->m_Lists = _ary[0];
  _Ary->m_Size = 0;
  free(_ary);
  return _Ary;
}
