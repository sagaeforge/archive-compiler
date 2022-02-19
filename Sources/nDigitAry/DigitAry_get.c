
#include <Module/nDigitAry.h>

DEFUALT_DIGIT
DigitAry_get(nDigitAry_ptr pSelf, Index_t pIndex)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;
  nDigitAryNode_ptr _Node = pSelf->m_Nodes;

  Index_t _i;
  for (_i = 0; _i < _index; _i++)
    _Node = _Node->Next;
  return _Node->m_Value;
}
