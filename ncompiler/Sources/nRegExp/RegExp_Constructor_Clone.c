
#include <Module/nString.h>
#include <Module/nStringAry.h>

nRegExp_t*
RegExp_Constructor_Clone(const nRegExp_t* pSelf)
{
  nString_ptr _FlagText = nString("");
  nStringAry_ptr _Temp = nStringAry(7, nString("d"), nString("g"), nString("i"), nString("m"), nString("s"), nString("u"), nString("y"));

  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_HasIndices))
    String_Append(_FlagText, StringAry_get(_Temp, 0));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_Global))
    String_Append(_FlagText, StringAry_get(_Temp, 1));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_IgnoreCase))
    String_Append(_FlagText, StringAry_get(_Temp, 2));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_Multiline))
    String_Append(_FlagText, StringAry_get(_Temp, 3));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_DotAll))
    String_Append(_FlagText, StringAry_get(_Temp, 4));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_Unicode))
    String_Append(_FlagText, StringAry_get(_Temp, 5));
  if (BitFlagCheck(pSelf->m_Flag, k_nRegExpFlag_Sticky))
    String_Append(_FlagText, StringAry_get(_Temp, 6));

  nRegExp_ptr _Reg = RegExp_Constructor_Str(pSelf->m_Pattern, _FlagText);
  Index_t _i;
  for (_i = 0; _i < 7; _i++) {
    nString_ptr _str = StringAry_get(_Temp, _i);
    String_Destructor(&_str);
  }
  StringAry_Destructor(&_Temp);
  String_Destructor(&_FlagText);
  return _Reg;
}
