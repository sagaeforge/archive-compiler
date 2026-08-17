
#include <Private_JsonAry.h>

bool
JSONAry_ParentType(const JSONAry pSelf)
{
  return pSelf->m_Parent.IsObject;
}