
#include <Module/nString.h>

int64_t
String_ValueOf_Decimal(const nString_t* pSelf)
{
  if (pSelf->m_Value[0] != '-')
    return String_ValueOf_Decimal_Unsigned(pSelf);

  uint64_t ret = 0;
  int i;
  for (i = 1; i < pSelf->m_Length - 1; i++) {
    ret += pSelf->m_Value[i] - L'0';
    ret *= 10;
  }
  ret += pSelf->m_Value[i] - L'0';
  return ret;
}
