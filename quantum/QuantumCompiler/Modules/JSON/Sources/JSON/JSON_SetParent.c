
#include <Private_Json.h>

bool
JSON_SetParent(JSONObject pSelf, const JSONObject pParent)
{
  pSelf->m_Parent = pParent;
  return true;
}
