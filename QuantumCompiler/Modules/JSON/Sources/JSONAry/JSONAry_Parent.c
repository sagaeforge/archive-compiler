
#include <Private_JsonAry.h>

void*
JSONAry_Parent(const JSONAry pSelf)
{
  return pSelf->m_Parent.m_Object;
}
