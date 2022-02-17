
#include <stdlib.h>

#include <Module/nStringAry.h>

bool
StringAry_Remove(nStringAry_t* pSelf, const Index_t pIndex)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;

  Index_t _i;
  if (pSelf->m_AryType == k_nStringAry_Ary) {
    pSelf->m_Arys[_index] = NULL;
    for (_i = _index; _i < pSelf->m_Size - 1; _i++) {
      nString_ptr _temp = pSelf->m_Arys[_i];
      pSelf->m_Arys[_i] = pSelf->m_Arys[_i + 1];
      pSelf->m_Arys[_i + 1] = _temp;
    }
  } else {
    nStringAryNode_ptr _node = pSelf->m_Lists;
    nStringAryNode_ptr _back = pSelf->m_Lists;

    for (_i = 0; _i < _index - 1; _i++) {
      _back = _node;
      _node = _node->Next;
    }

    _back->Next = _node->Next;
    free(_node);
  }

  pSelf->m_Length--;
  return true;
}
