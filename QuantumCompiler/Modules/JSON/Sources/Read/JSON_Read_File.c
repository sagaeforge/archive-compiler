
#include <Json.h>
#include <Private_Json.h>
#include <StringLib.h>

bool
JSON_Read_File(JSONObject pSelf, const FILE* pJsonFile)
{
  StringAry Ary = StringLibMethod.FileAllRead((FILE*)pJsonFile);
  return JSON_Read_StrAry(pSelf, Ary);
}
