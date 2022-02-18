
#include <Module/nStringAry.h>

bool
StringAry_Insert(nStringAry_t* pSelf, const nString_t* pValue, Index_t pIndex)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;

  Index_t _i;
  if (pSelf->m_AryType == k_nStringAry_Ary) {
    if (pSelf->m_Size == pSelf->m_Length)
      return false;

    for (_i = pSelf->m_Size - 1; _i >= _index; _i--) {
      nString_ptr _temp = pSelf->m_Arys[_i];
      pSelf->m_Arys[_i] = pSelf->m_Arys[_i - 1];
      pSelf->m_Arys[_i - 1] = _temp;
    }
    pSelf->m_Arys[_index] = (nString_ptr)pValue;
  } else {
    nStringAryNode_ptr _node = pSelf->m_Lists;
    nStringAryNode_ptr _makeNode = StringAry_MakeNode();
    _makeNode->m_Value = (nString_ptr)pValue;

    for (_i = 0; _i < _index - 1; _i++)
      _node = _node->Next;

    _makeNode->Next = _node->Next;
    _node->Next = _makeNode;
  }

  pSelf->m_Length++;
  return true;
}
