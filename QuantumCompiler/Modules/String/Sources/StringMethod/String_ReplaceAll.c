
#include <Chs.h>
#include <Private_String.h>

String
String_ReplaceAll(String pSelf, String pOri, String pValue)
{
  Length_t calcvalue = String_Count(pSelf, pOri);
  if (calcvalue == 0)
    return String(pSelf);
  Length_t leng =
    pSelf->Length - (pOri->Length * calcvalue) + (pValue->Length * calcvalue);
  wcs temp = __WcsCreate(leng);
  bool IsNull = String_IsNone(pValue);
  int i, j, temp_Pos = 0;
  for (i = 0; i < pSelf->Length; i++)
    if (_StringCompare(pSelf->Value, pOri, i)) {
      if (!IsNull) {
        for (j = 0; j < pValue->Length; j++)
          temp[i + j + temp_Pos] = pValue->Value[j];
        i += pOri->Length;
      } else {
        temp_Pos--;
        continue;
      }
    } else
      temp[i + temp_Pos] = pSelf->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
