
#include <Chs.h>
#include <Private_String.h>

String
String_ReplaceFor(String pSelf, String pOri, String pValue, Length_t pLength)
{
  if (pLength > String_Count(pSelf, pOri) || pLength == 0)
    return String(pSelf);

  int leng =
    pSelf->Length - (pOri->Length * pLength) + (pValue->Length * pLength);
  wcs temp = __WcsCreate(leng);
  bool IsNull = String_IsNone(pValue);
  int i, j, total = 0, temp_Pos = 0;
  for (i = 0; i < pSelf->Length; i++)
    if (total < pLength && _StringCompare(pSelf->Value, pOri, i)) {
      if (!IsNull) {
        for (j = 0; j < pValue->Length; j++)
          temp[i + j + temp_Pos] = pValue->Value[j];
        i += pOri->Length;
      } else {
        temp_Pos--;
        total++;
        continue;
      }
      total++;
    } else
      temp[i + temp_Pos] = pSelf->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
