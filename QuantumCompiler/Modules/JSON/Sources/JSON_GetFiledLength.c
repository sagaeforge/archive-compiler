
#include <Json.h>
#include <Private_Json.h>

Length_t
JSON_GetFiledLength(const JSONObject pSelf)
{
  return pSelf->m_FieldLength;
}
