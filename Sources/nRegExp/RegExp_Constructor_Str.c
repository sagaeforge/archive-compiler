
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nRegExp_t*
RegExp_Constructor_Str(nString_t* pRegExp, nString_t* pFlag)
{
  nRegExp_ptr _Exp = malloc(sizeof(nRegExp_t));
  if (!_Exp) {
    Exception(ERROR, "정규식 객체를 만들수 없습니다.");
    return NULL;
  }

  _Exp->m_Pattern = nString(pRegExp);
  _Exp->m_Flag = k_nRegExpFlag_None;

  Index_t _i;
  for (_i = 0; _i < pFlag->m_Length; _i++)
    switch (__ToLower(pFlag->m_Value[_i])) {
      // clang-format off
      case L'd': _Exp->m_Flag |= k_nRegExpFlag_HasIndices;  break;
      case L'g': _Exp->m_Flag |= k_nRegExpFlag_Global;      break;
      case L'i': _Exp->m_Flag |= k_nRegExpFlag_IgnoreCase;  break;
      case L'm': _Exp->m_Flag |= k_nRegExpFlag_Multiline;   break;
      case L's': _Exp->m_Flag |= k_nRegExpFlag_DotAll;      break;
      case L'u': _Exp->m_Flag |= k_nRegExpFlag_Unicode;     break;
      case L'y': _Exp->m_Flag |= k_nRegExpFlag_Sticky;      break;
      // clang-format on
      default:
        Exception(ERROR, "알 수 없는 플래그입니다. [flag:%c]", pFlag->m_Value[_i]);
        return NULL;
    }

  return _Exp;
}
