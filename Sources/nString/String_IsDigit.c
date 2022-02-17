
#include <Module/nString.h>

bool
String_IsDigit(const nString_t* pSelf)
{
  int _i, _DotCount = 0, _ECount = 0;
  for (_i = 0; _i < pSelf->m_Length; _i++) {
    if (pSelf->m_Value[_i] == '.') {
      if (_DotCount != 0)
        return false;
      _DotCount++;
    } else if (pSelf->m_Value[_i] == 'E') {
      if (_ECount != 0)
        return false;
      _ECount++;
    } else if (pSelf->m_Value[_i] == '+') {
      if (_i != 0 || (_i != 0 && pSelf->m_Value[_i - 1] != 'E'))
        return false;
    } else if (pSelf->m_Value[_i] == '-') {
      if (_i != 0 || (_i != 0 && pSelf->m_Value[_i - 1] != 'E'))
        return false;
    } else if (!__IsDecimal(pSelf->m_Value[_i]))
      return false;
  }
  return true;
}
