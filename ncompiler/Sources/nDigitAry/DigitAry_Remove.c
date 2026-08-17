
#include <stdio.h>
#include <stdlib.h>

#include <Module/nDigitAry.h>

void
DigitAry_Remove(nDigitAry_ptr pSelf, Index_t pIndex)
{
  if (pSelf->m_Length == 0)
    return;

  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;
  nDigitAryNode_ptr _Node = pSelf->m_Nodes;
  nDigitAryNode_ptr _Back = pSelf->m_Nodes;

  Index_t _i;
  for (_i = 0; _i < _index; _i++) {
    _Back = _Node;
    _Node = _Node->Next;
  }

  free(_Node);
  _Back->Next = _Node->Next;
  pSelf->m_Length--;
}