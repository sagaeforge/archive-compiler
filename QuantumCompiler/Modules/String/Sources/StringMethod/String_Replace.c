
#include "Chs.h"
#include "Private_String.h"

String
String_Replace(String Self, String Ori, String Value)
{
  int ind = String_IndexOf(Self, Ori);
  if (ind == -1)
    return String(Self);

  bool IsNull = String_IsNone(Value);
  Length_t leng = Self->Length - Ori->Length + Value->Length;
  wcs temp = __WcsCreate(leng);
  int i, j, temp_Pos = 0;
  for (i = 0; i < leng; i++)
    if (i == ind) {
      if (!IsNull) {
        for (j = 0; j < Value->Length; j++)
          temp[i + j + temp_Pos] = Value->Value[j];
        i += Ori->Length;
      } else {
        temp_Pos--;
        continue;
      }
    } else
      temp[i + temp_Pos] = Self->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
