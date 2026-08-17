
#include <Private_Json.h>
#include <String.h>
#include <StringAry.h>

bool
JSON_FieldOf(const JSONObject pSelf, const String pFiledName)
{
  Index_t i;
  for (i = 0; i < pSelf->m_FieldLength; i++)
    if (StringMethod.Compare(StringAryMethod.Get(pSelf->m_FieldNames, i),
                             pFiledName))
      return true;
  return false;
}
