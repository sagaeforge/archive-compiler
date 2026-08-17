
#include <stdlib.h>

#include <Module/nString.h>

Chs_t
UTF8_Encoder(Wcs_t pValue, Length_t* out_pValueSize)
{
  int _i = 0, _Length = 0;
  Index_t _Len = __STRLEN(pValue);
  for (_i = 0; _i < _Len; _i++) {
    // 1바이트인 경우
    // 000000-00007F
    if ((uint32_t)pValue[_i] < 0x7F)
      _Length += 1;
    // 2바이트인 경우
    // 000080-0007FF
    else if ((uint32_t)pValue[_i] >= 0x80 && (uint32_t)pValue[_i] <= 0x7FF)
      _Length += 2;
    // 3바이트인 경우
    // 000800-00FFFF
    else if ((uint32_t)pValue[_i] >= 0x800 && (uint32_t)pValue[_i] <= 0xFFFF)
      _Length += 3;
    // 4바이트인 경우
    // 010000-10FFFF
    else if ((uint32_t)pValue[_i] >= 0x10000 && (uint32_t)pValue[_i] <= 0x10FFFF)
      _Length += 4;
  }

  (*out_pValueSize) = _Length;
  Chs_t Temp = (Chs_t)calloc(1, _Length + 1);
  if (!Temp) {
    return NULL;
  }

  int _LengthPointer = 0;
  for (_i = 0; _i < _Len; _i++) {
    // 1바이트인 경우
    // 000000-00007F
    if ((uint32_t)pValue[_i] < 0x7F)
      Temp[_LengthPointer++] = pValue[_i];
    // 2바이트인 경우
    // 000080-0007FF
    else if ((uint32_t)pValue[_i] >= 0x80 && (uint32_t)pValue[_i] <= 0x7FF) {
      Temp[_LengthPointer++] = (0b110 << 5) | (pValue[_i] & (BitAndMask(5) << 6));
      Temp[_LengthPointer++] = (0b10 << 6) | (pValue[_i] & BitAndMask(6));
    }
    // 3바이트인 경우
    // 000800-00FFFF
    else if ((uint32_t)pValue[_i] >= 0x800 && (uint32_t)pValue[_i] <= 0xFFFF) {
      Temp[_LengthPointer++] = (0b1110 << 4) | ((pValue[_i] & (BitAndMask(4) << 12)) >> 12);
      Temp[_LengthPointer++] = (0b10 << 6) | ((pValue[_i] & (BitAndMask(6) << 6)) >> 6);
      Temp[_LengthPointer++] = (0b10 << 6) | (pValue[_i] & BitAndMask(6));
    }
    // 4바이트인 경우
    // 010000-10FFFF
    else if ((uint32_t)pValue[_i] >= 0x10000 && (uint32_t)pValue[_i] <= 0x10FFFF) {
      Temp[_LengthPointer++] = (0b11110 << 3) | ((pValue[_i] & (BitAndMask(3) << 18)) >> 18);
      Temp[_LengthPointer++] = (0b10 << 6) | ((pValue[_i] & (BitAndMask(6) << 12)) >> 12);
      Temp[_LengthPointer++] = (0b10 << 6) | ((pValue[_i] & (BitAndMask(6) << 6)) >> 6);
      Temp[_LengthPointer++] = (0b10 << 6) | (pValue[_i] & BitAndMask(6));
    }
  }

  return Temp;
}
