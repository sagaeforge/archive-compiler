
#include <Module/nString.h>
#include <Module/nStringAry.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#pragma region 디버깅용
Func_t Debugging[] = { (Func_t)String_Constructor_Chs,
                       (Func_t)String_Constructor_Wcs,
                       (Func_t)String_Constructor_Str,
                       (Func_t)String_Constructor_Strp,
                       (Func_t)String_Destructor,
                       (Func_t)String_Join,
                       (Func_t)String_Append,
                       (Func_t)String_SubString,
                       (Func_t)String_Loop,
                       (Func_t)String_Split,
                       (Func_t)String_Compare,
                       (Func_t)String_Trim,
                       (Func_t)String_Contains,
                       (Func_t)String_Count,
                       (Func_t)String_Get,
                       (Func_t)String_Set,
                       (Func_t)String_Length,
                       (Func_t)String_ToLower,
                       (Func_t)String_ToUpper,
                       (Func_t)String_IndexOf,
                       (Func_t)String_IndexAt,
                       (Func_t)String_IndexFor,
                       (Func_t)String_LastOfIndex,
                       (Func_t)String_Replace,
                       (Func_t)String_ReplaceAt,
                       (Func_t)String_ReplaceAll,
                       (Func_t)String_Left,
                       (Func_t)String_Right,
                       (Func_t)String_Middle,
                       (Func_t)String_Extract,
                       (Func_t)String_Reverse,
                       (Func_t)String_Search,
                       (Func_t)String_IsAlpha,
                       (Func_t)String_IsLower,
                       (Func_t)String_IsUpper,
                       (Func_t)String_IsDecimal,
                       (Func_t)String_IsDigit,
                       (Func_t)String_IsSpace,
                       (Func_t)String_IsAlphaDigit,
                       (Func_t)String_IsHex,
                       (Func_t)String_IsControl,
                       (Func_t)String_IsOctal,
                       (Func_t)String_IsBinary,
                       (Func_t)String_Format,
                       (Func_t)String_Pattern,
                       (Func_t)String_Check,
                       (Func_t)String_Notation,
                       (Func_t)String_FileAllRead,
                       (Func_t)String_FileAllWrite,
                       (Func_t)String_Print,
                       (Func_t)String_PrintErr,
                       (Func_t)String_PrintLine,
                       (Func_t)String_ToString_Bool,
                       (Func_t)String_ToString_Decimal,
                       (Func_t)String_ToString_Decimal_Unsigned,
                       (Func_t)String_ToString_Digit,
                       (Func_t)String_ToString_StringAry,
                       (Func_t)String_ValueOf_Bool,
                       (Func_t)String_ValueOf_Decimal,
                       (Func_t)String_ValueOf_Decimal_Unsigned,
                       (Func_t)String_ValueOf_Digit,
                       (Func_t)StringAry_Constructor,
                       (Func_t)StringAry_Destructor,
                       (Func_t)StringAry_Get,
                       (Func_t)StringAry_Insert,
                       (Func_t)StringAry_Remove,
                       (Func_t)StringAry_Push,
                       (Func_t)StringAry_Pop,
                       (Func_t)StringAry_Search,
                       (Func_t)StringAry_Contains,
                       (Func_t)StringAry_toAry,
                       (Func_t)StringAry_toList,
                       (Func_t)StringAry_isAry,
                       (Func_t)StringAry_isList,
                       (Func_t)StringAry_Foreach,
                       (Func_t)StringAry_CountIf };
#pragma endregion

Length_t
_ChsLen(Chs_t pValue, size_t pValueSize)
{
  int i = 0, Length = 0;
  for (i = 0; i < pValueSize; i++) {
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

Wcs_t
UTF8_Decorder(Chs_t pValue, size_t pValueSize)
{
  Wcs_t Temp = __WCSMAKE(_ChsLen(pValue, pValueSize));
  if (!Temp) {
    return NULL;
  }
  int i, LengthPointer = 0, garbage = 0;
  for (i = 0; i < pValueSize; i++) {
    // 1바이트인 경우
    // 0000|0000 ~ 0111|1111
    if ((uint8_t)pValue[i] < 128) {
      Temp[LengthPointer++] = pValue[i];
    }
    // 2바이트인 경우
    // 1000|0000 ~ 1101|1111
    else if ((uint8_t)pValue[i] >= 128 && (uint8_t)pValue[i] <= 223) {
      garbage = pValue[i] & 0b1111;
      garbage <<= 6;
      garbage += pValue[i + 1] & 0b111111;
      Temp[LengthPointer++] = garbage;
      i += 1;
    }
    // 3바이트인 경우
    // 1110|0000 ~ 1110|1111
    else if ((uint8_t)pValue[i] >= 224 && (uint8_t)pValue[i] <= 239) {
      garbage = pValue[i] & 0b1111;
      garbage <<= 6;
      garbage += pValue[i + 1] & 0b111111;
      garbage <<= 6;
      garbage += pValue[i + 2] & 0b111111;
      Temp[LengthPointer++] = garbage;
      i += 2;
    }
    // 4바이트인 경우
    // 1111|0000 ~ 1111|0111
    else if ((uint8_t)pValue[i] >= 240 && (uint8_t)pValue[i] <= 247) {
      garbage = pValue[i] & 0b1111;
      garbage <<= 6;
      garbage += pValue[i + 1] & 0b111111;
      garbage <<= 6;
      garbage += pValue[i + 2] & 0b111111;
      garbage <<= 6;
      garbage += pValue[i + 3] & 0b111111;
      Temp[LengthPointer++] = garbage;
      i += 3;
    }
  }

  return Temp;
}

Chs_t
UTF8_Encoder(Wcs_t pValue, size_t* out_pValueSize)
{
  int i = 0, Length = 0;
  for (i = 0; i < wcslen(pValue); i++) {
    // 1바이트인 경우
    // 000000-00007F
    if ((uint32_t)pValue[i] < 0x7F) {
      Length += 1;
    }
    // 2바이트인 경우
    // 000080-0007FF
    else if ((uint32_t)pValue[i] >= 0x80 && (uint32_t)pValue[i] <= 0x7FF) {
      Length += 2;
    }
    // 3바이트인 경우
    // 000800-00FFFF
    else if ((uint32_t)pValue[i] >= 0x800 && (uint32_t)pValue[i] <= 0xFFFF) {
      Length += 3;
    }
    // 4바이트인 경우
    // 010000-10FFFF
    else if ((uint32_t)pValue[i] >= 0x10000 && (uint32_t)pValue[i] <= 0x10FFFF) {
      Length += 4;
    }
  }
  (*out_pValueSize) = Length;
  Chs_t Temp = (Chs_t)calloc(1, Length + 1);
  if (!Temp) {
    return NULL;
  }
  int LengthPointer = 0;
  for (i = 0; i < wcslen(pValue); i++) {
    // 1바이트인 경우
    // 000000-00007F
    if ((uint32_t)pValue[i] < 0x7F) {
      Temp[LengthPointer++] = pValue[i];
    }
    // 2바이트인 경우
    // 000080-0007FF
    else if ((uint32_t)pValue[i] >= 0x80 && (uint32_t)pValue[i] <= 0x7FF) {
      Temp[LengthPointer++] = (0b110 << 5) | pValue[i] & (BitAndMask(5) << 6);
      Temp[LengthPointer++] = (0b10 << 6) | pValue[i] & BitAndMask(6);
    }
    // 3바이트인 경우
    // 000800-00FFFF
    else if ((uint32_t)pValue[i] >= 0x800 && (uint32_t)pValue[i] <= 0xFFFF) {
      Temp[LengthPointer++] = (0b1110 << 4) | ((pValue[i] & (BitAndMask(4) << 12)) >> 12);
      Temp[LengthPointer++] = (0b10 << 6) | ((pValue[i] & (BitAndMask(6) << 6)) >> 6);
      Temp[LengthPointer++] = (0b10 << 6) | (pValue[i] & BitAndMask(6));
    }
    // 4바이트인 경우
    // 010000-10FFFF
    else if ((uint32_t)pValue[i] >= 0x10000 && (uint32_t)pValue[i] <= 0x10FFFF) {
      Temp[LengthPointer++] = (0b11110 << 3) | ((pValue[i] & (BitAndMask(3) << 18)) >> 18);
      Temp[LengthPointer++] = (0b10 << 6) | ((pValue[i] & (BitAndMask(6) << 12)) >> 12);
      Temp[LengthPointer++] = (0b10 << 6) | ((pValue[i] & (BitAndMask(6) << 6)) >> 6);
      Temp[LengthPointer++] = (0b10 << 6) | (pValue[i] & BitAndMask(6));
    }
  }
  return Temp;
}

int
main(int argc, char const* argv[])
{
  setlocale(LC_ALL, "");
  Chs_t Debug = "가나다라adasdsdasㄴㅁ암ㄴㅇㅁ너";
  for (size_t i = 0; i < sizeof(Debug); i++)
    printf("ary[%2lu]=%u\n", i, (uint8_t)Debug[i]);

  Wcs_t value = L"가나다라adasdsdasㄴㅁ암ㄴㅇㅁ너";
  size_t Length = 0;
  Chs_t ary = UTF8_Encoder(value, &Length);

  // Wcs_t temp = Decorder_character(ary, sizeof(ary));
  printf("%s\n", ary);

  free(ary);

  return 0;
}
