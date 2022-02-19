
#include <Module/nDigitAry.h>

void
DigitAry_set(nDigitAry_ptr pSelf, Index_t pIndex, DEFUALT_DIGIT pValue)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;
  nDigitAryNode_ptr _node = pSelf->m_Nodes;

  Index_t _i;
  for (_i = 0; _i < _index; _i++)
    _node = _node->Next;
  _node->m_Value = pValue;
}
