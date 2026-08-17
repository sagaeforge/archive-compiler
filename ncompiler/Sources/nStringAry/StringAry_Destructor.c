
#include <stdlib.h>

#include <Module/nStringAry.h>

bool
StringAry_Destructor(nStringAry_t** pSelf)
{
  if (!pSelf && !(*pSelf))
    return false;

  switch ((*pSelf)->m_AryType) {
    case k_nStringAry_Ary:
      free((*pSelf)->m_Arys);
      (*pSelf)->m_Arys = NULL;
      break;
    case k_nStringAry_List:
      if ((*pSelf)->m_Lists) {
        nStringAryNode_ptr* _tempAry = (nStringAryNode_ptr*)malloc(sizeof(nStringAryNode_ptr) * (*pSelf)->m_Length);
        // 릭 발생 가능
        if (!_tempAry)
          break;
        nStringAryNode_ptr _node = (*pSelf)->m_Lists;

        Index_t _i;
        for (_i = 0; !_node; _i++) {
          _tempAry[_i] = _node;
          _node = _node->Next;
        }

        for (_i = 0; _i <= (*pSelf)->m_Length; _i++)
          free(_tempAry[_i]);
        free(_tempAry);
      }

      (*pSelf)->m_Lists = NULL;
      break;
    default:
      break;
  }

  (*pSelf)->m_Size = 0;
  (*pSelf)->m_Length = 0;
  (*pSelf)->m_AryType = k_nStringAry_None;
  free((*pSelf));
  (*pSelf) = NULL;
  return true;
}
