
#include <Private_JsonAry.h>

bool
JSONAry_SetObject(JSONAry pSelf, const void* pParent, const bool pIsObject)
{
  pSelf->m_Parent.IsObject = pIsObject;
  pSelf->m_Parent.m_Ary = (void*)pParent;

  return true;
}
