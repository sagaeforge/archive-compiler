
#include <Module/nDigitAry.h>

nDigitAry_ptr
DigitAry_Split(nDigitAry_ptr pSelf, Index_t pIndex)
{
  nDigitAry_ptr _Ary = DigitAry_Constructor();
  if (!_Ary)
    return NULL;

  Index_t _index = pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;
  nDigitAryNode_ptr _SelfNode = pSelf->m_Nodes;
  nDigitAryNode_ptr _AryNode;
  Index_t _i;
  for (_i = 0; _i < _index - 1; _i++) {
    nDigitAryNode_ptr _Make = DigitAry_MakeNode();
    _Make->m_Value = _SelfNode->m_Value;

    if (_i == 0)
      _Ary->m_Nodes = _Make;
    else
      _AryNode->Next = _Make;

    _AryNode = _Make;
    _SelfNode = _SelfNode->Next;
  }

  _Ary->m_Length = _index - 1;
  return _Ary;
}
