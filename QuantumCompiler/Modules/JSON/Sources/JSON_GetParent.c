
#include <Json.h>
#include <Private_Json.h>

JSONObject
JSON_GetParent(const JSONObject pSelf)
{
  return pSelf->m_Parent;
}