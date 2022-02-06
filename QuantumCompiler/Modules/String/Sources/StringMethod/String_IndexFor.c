
#include <Private_String.h>

Index_t
String_IndexFor(String pSelf, String pValue, Index_t pIndex)
{
  Length_t len = String_Count(pSelf, pValue);
  if (len == 0 || len <= pIndex)
    return -1;

  int i, cnt = 0;
  for (i = 0; i < pSelf->m_Length; i++) {
    if (pSelf->m_Value[i] == pValue->m_Value[0])
      if (_StringCompare(pSelf->m_Value, pValue, i)) {
        if (cnt != pIndex)
          cnt++;
        else
          return i;
      }
  }
  return -1;
}