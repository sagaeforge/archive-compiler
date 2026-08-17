
#include <Module/nStringAry.h>

nString_t*
StringAry_get(const nStringAry_t* pSelf, const Index_t pIndex)
{
  Index_t _index = pIndex <= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;

  Index_t _i = 0;
  nStringAryNode_ptr _node = pSelf->m_Lists;
  switch (pSelf->m_AryType) {
    case k_nStringAry_Ary:
      return pSelf->m_Arys[_index];
    case k_nStringAry_List:
      for (_i = 0; _i < _index; _i++)
        _node = _node->Next;
      return _node->m_Value;
    default:
      break;
  }

  return NULL;
}
