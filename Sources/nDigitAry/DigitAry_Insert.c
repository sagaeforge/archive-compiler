
#include <Module/nDigitAry.h>

void
DigitAry_Insert(nDigitAry_ptr pSelf, Index_t pIndex, DEFUALT_DIGIT pValue)
{
  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;
  nDigitAryNode_ptr _Node = pSelf->m_Nodes;
  nDigitAryNode_ptr _Make = DigitAry_MakeNode();
  _Make->m_Value = pValue;

  pSelf->m_Length++;

  if (_index == 0) {
    pSelf->m_Nodes = _Make;
    return;
  }

  Index_t _i;
  for (_i = 0; _i < _index; _i++)
    _Node = _Node->Next;
  _Make->Next = _Node->Next;
  _Node->Next = _Make;
}
