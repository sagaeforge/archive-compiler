
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <StringAry.h>

bool
JSON_Destructor(const JSONObject* pSelf)
{
  if (pSelf == NULL || (*pSelf) == NULL)
    return false;

  // TODO 해야함.

  return true;
}
