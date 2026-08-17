
#include <Module/nString.h>

Length_t
__ChsLen(const Chs_t pValue)
{
  int i = 0, Length = 0;
  for (i = 0; pValue[i] != '\0'; i++) {
    Length++;
    // 1바이트인 경우
    // 0000|0000 ~ 0111|1111
    if ((uint8_t)pValue[i] < 128) {
      i += 0;
    }
    // 2바이트인 경우
    // 1000|0000 ~ 1101|1111
    else if ((uint8_t)pValue[i] >= 128 && (uint8_t)pValue[i] <= 223) {
      i += 1;
    }
    // 3바이트인 경우
    // 1110|0000 ~ 1110|1111
    else if ((uint8_t)pValue[i] >= 224 && (uint8_t)pValue[i] <= 239) {
      i += 2;
    }
    // 4바이트인 경우
    // 1111|0000 ~ 1111|0111
    else if ((uint8_t)pValue[i] >= 240 && (uint8_t)pValue[i] <= 247) {
      i += 3;
    }
  }
  return Length;
}