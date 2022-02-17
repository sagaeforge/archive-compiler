
#include <Module/nStringAry.h>

bool
StringAry_set(const nStringAry_t* pSelf, const Index_t pIndex, const nString_t* pValue)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;

  Index_t _i;
  if (pSelf->m_AryType == k_nStringAry_Ary) {
    pSelf->m_Arys[_index] = (nString_ptr)pValue;
  } else {
    nStringAryNode_ptr _node = pSelf->m_Lists;
    for (_i = 0; _i < _index; _i++)
      _node = _node->Next;

    _node->m_Value = (nString_ptr)pValue;
  }

  return true;
}