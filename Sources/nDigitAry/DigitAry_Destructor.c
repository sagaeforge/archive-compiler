
#include <stdio.h>
#include <stdlib.h>

#include <Module/nDigitAry.h>

void
DigitAry_Destructor(nDigitAry_ptr* pSelf)
{
  if (!pSelf && !(*pSelf))
    return;

  nDigitAryNode_ptr* _tempAry = (nDigitAryNode_ptr*)malloc(sizeof(nDigitAryNode_ptr) * (*pSelf)->m_Length);
  // 릭 발생 가능
  if (!_tempAry)
    return;

  nDigitAryNode_ptr _node = (*pSelf)->m_Nodes;
  Index_t _i;
  for (_i = 0; !_node; _i++) {
    _tempAry[_i] = _node;
    _node = _node->Next;
  }

  for (_i = 0; _i <= (*pSelf)->m_Length; _i++)
    free(_tempAry[_i]);
  free(_tempAry);

  (*pSelf)->m_Nodes = NULL;
  (*pSelf)->m_Length = 0;
  free((*pSelf));
  (*pSelf) = NULL;
}
