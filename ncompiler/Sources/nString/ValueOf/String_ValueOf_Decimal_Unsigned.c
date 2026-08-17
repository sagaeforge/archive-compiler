
#include <Module/nString.h>

uint64_t
String_ValueOf_Decimal_Unsigned(const nString_t* pSelf)
{
  uint64_t ret = 0;
  int i;
  for (i = 0; i < pSelf->m_Length - 1; i++) {
    ret += pSelf->m_Value[i] - L'0';
    ret *= 10;
  }
  ret += pSelf->m_Value[i] - L'0';
  return ret;
}
