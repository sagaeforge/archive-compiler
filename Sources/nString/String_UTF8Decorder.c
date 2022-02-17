
#include <stdlib.h>

#include <Module/nString.h>

Wcs_t
String_UTF8Decorder(Chs_t pValue, Length_t* out_pValueSize)
{
  Length_t _Len;
  _Len = (*out_pValueSize) = __STRLEN(pValue);
  Wcs_t _Temp = __WCSMAKE(_Len);
  if (!_Temp) {
    return NULL;
  }

  int _i, _LengthPointer = 0, _garbage = 0;
  for (_i = 0; pValue[_i] != '\0'; _i++) {
    // 1바이트인 경우
    // 0000|0000 ~ 0111|1111
    if ((uint8_t)pValue[_i] < 128)
      _Temp[_LengthPointer++] = pValue[_i];
    // 2바이트인 경우
    // 1000|0000 ~ 1101|1111
    else if ((uint8_t)pValue[_i] >= 128 && (uint8_t)pValue[_i] <= 223) {
      _garbage = pValue[_i] & 0b1111;
      _garbage <<= 6;
      _garbage += pValue[_i + 1] & 0b111111;
      _Temp[_LengthPointer++] = _garbage;
      _i += 1;
    }
    // 3바이트인 경우
    // 1110|0000 ~ 1110|1111
    else if ((uint8_t)pValue[_i] >= 224 && (uint8_t)pValue[_i] <= 239) {
      _garbage = pValue[_i] & 0b1111;
      _garbage <<= 6;
      _garbage += pValue[_i + 1] & 0b111111;
      _garbage <<= 6;
      _garbage += pValue[_i + 2] & 0b111111;
      _Temp[_LengthPointer++] = _garbage;
      _i += 2;
    }
    // 4바이트인 경우
    // 1111|0000 ~ 1111|0111
    else if ((uint8_t)pValue[_i] >= 240 && (uint8_t)pValue[_i] <= 247) {
      _garbage = pValue[_i] & 0b1111;
      _garbage <<= 6;
      _garbage += pValue[_i + 1] & 0b111111;
      _garbage <<= 6;
      _garbage += pValue[_i + 2] & 0b111111;
      _garbage <<= 6;
      _garbage += pValue[_i + 3] & 0b111111;
      _Temp[_LengthPointer++] = _garbage;
      _i += 3;
    }
  }

  return _Temp;
}