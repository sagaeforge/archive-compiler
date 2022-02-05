
#include <Json.h>
#include <StringLib.h>

bool
JSON_Read_StrAry(JSONObject pSelf, const StringAry pStringAry)
{
  String str = toString(pStringAry, String("\n"));
  return JSON_Read_Str(pSelf, str);
}
