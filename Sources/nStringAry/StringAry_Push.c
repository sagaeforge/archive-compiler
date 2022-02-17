
#include <Module/__nStringAry.h>
#include <Module/nStringAry.h>

bool
StringAry_Push(nStringAry_t* pSelf, const nString_t* pValue)
{
  if (pSelf->m_AryType == k_nStringAry_Ary) {
    if (pSelf->m_Size == pSelf->m_Length)
      return false;

    pSelf->m_Arys[pSelf->m_Length - 1] = (nString_ptr)pValue;
  } else {
    nStringAryNode_ptr _node = pSelf->m_Lists;
    while (_node->Next != NULL)
      _node = _node->Next;
    _node->Next = StringAry_MakeNode();
    _node->Next->m_Value = (nString_ptr)pValue;
  }
  pSelf->m_Length++;
  return true;
}
