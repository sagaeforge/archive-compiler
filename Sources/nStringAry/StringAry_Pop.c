
#include <stdlib.h>

#include <Module/__nStringAry.h>
#include <Module/nStringAry.h>

nString_t*
StringAry_Pop(nStringAry_t* pSelf)
{
  if (pSelf->m_Length != 0) {
    nString_ptr _str;
    if (pSelf->m_AryType == k_nStringAry_Ary) {
      _str = pSelf->m_Arys[pSelf->m_Length - 1];
      pSelf->m_Arys[pSelf->m_Length - 1] = NULL;
    } else {
      nStringAryNode_ptr _node = pSelf->m_Lists;
      nStringAryNode_ptr _back = pSelf->m_Lists;
      while (_node->Next != NULL) {
        _back = _node;
        _node = _node->Next;
      }
      _str = _node->m_Value;
      _back->Next = NULL;
      free(_node);
    }

    pSelf->m_Length--;
    return _str;
  }
  return NULL;
}
