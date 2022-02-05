
#include <Private_JsonAry.h>

bool
JSONAry_SetObject(const JSONAry pSelf, const JSONObject pObject)
{
  pSelf->m_Object = pObject;
  return true;
}
