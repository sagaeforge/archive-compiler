
#include <Chs.h>
#include <Private_String.h>

String
String_ReplaceAll(String pSelf, String pOri, String pValue)
{
  Length_t calcvalue = String_Count(pSelf, pOri);
  if (calcvalue == 0)
    return String(pSelf);
  Length_t leng = pSelf->m_Length - (pOri->m_Length * calcvalue) +
                  (pValue->m_Length * calcvalue);
  wcs temp = __WcsCreate(leng);
  bool IsNull = String_IsNone(pValue);
  int i, j, temp_Pos = 0;
  for (i = 0; i < pSelf->m_Length; i++)
    if (_StringCompare(pSelf->m_Value, pOri, i)) {
      if (!IsNull) {
        for (j = 0; j < pValue->m_Length; j++)
          temp[i + j + temp_Pos] = pValue->m_Value[j];
        i += pOri->m_Length;
      } else {
        temp_Pos--;
        continue;
      }
    } else
      temp[i + temp_Pos] = pSelf->m_Value[i];
  temp[i] = L'\0';
  return String(temp);
}
