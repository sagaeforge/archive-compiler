
#include <Chs.h>
#include <Private_String.h>

String
String_Replace(String pSelf, String pOri, String pValue)
{
  int ind = String_IndexOf(pSelf, pOri);
  if (ind == -1)
    return String(pSelf);

  bool IsNull = String_IsNone(pValue);
  Length_t leng = pSelf->Length - pOri->Length + pValue->Length;
  wcs temp = __WcsCreate(leng);
  int i, j, temp_Pos = 0;
  for (i = 0; i < leng; i++)
    if (i == ind) {
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
