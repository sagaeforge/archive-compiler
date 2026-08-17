
#include <GarbageCollection.h>
#include <Private_StringAry.h>

Length_t
StringAry_Contains(StringAry pSelf, String pValue)
{
  return StringAry_Search(pSelf, pValue) != -1;
}