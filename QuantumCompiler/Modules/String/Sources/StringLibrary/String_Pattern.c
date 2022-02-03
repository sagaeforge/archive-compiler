
#include <Chs.h>
#include <Exception.h>
#include <Private_StringLib.h>

#include <stdio.h>

bool
String_Pattern(String pSelf, String pFormat)
{
  int i;
  int gap = 0;
  int dotCount = 0, ECount = 0;
  int start;
  for (i = 0; i < pFormat->Length; i++) {
    if (pFormat->Value[i] == '%') {
      i++;
      gap -= 1;
      // clang-format off
      switch (pFormat->Value[i]) {
      // '%'일 때
      case '%':
        if (pSelf->Value[i + gap] != '%')
          return false;
        break;
      // 문자일 때
      case 's': case 'S':
        if (pSelf->Value[i + gap] != '\"')
          return false;
        gap++;
        while (pSelf->Value[i + gap] != '\"' && pSelf->Value[i + gap] != '\0')
          gap++;

        if (pSelf->Value[i + gap] == '\0')
          return false;
        break;
      // 정수일 때
      case 'd': case 'D':
        while (__IsDecimal(pSelf->Value[i + gap]) && pSelf->Value[i + gap] != '\0')
          gap++;
        break;
      // 16진수일 때
      case 'x': case 'X':
        if (pSelf->Value[i + gap] != '0' && (pSelf->Value[i + gap + 1] != 'x' ||
                                            pSelf->Value[i + gap + 1] != 'X'))
          return false;
        gap += 2;
        while (__IsHex(pSelf->Value[i + gap]) && pSelf->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 2진수일 때
      case 'b': case 'B':
        if (pSelf->Value[i + gap] != '0' && (pSelf->Value[i + gap + 1] != 'b' ||
                                            pSelf->Value[i + gap + 1] != 'B'))
          return false;
        gap += 2;
        while (__IsBinary(pSelf->Value[i + gap]) && pSelf->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 8진수일 때
      case 'o':
      case 'O':
        if (pSelf->Value[i + gap] != '0' && (pSelf->Value[i + gap + 1] != 'o' ||
                                            pSelf->Value[i + gap + 1] != 'O'))
          return false;
        gap += 2;
        while (__IsOctal(pSelf->Value[i + gap]) && pSelf->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      case 'f': case 'F':
        start = i + gap;
        while (true) {
          if (pSelf->Value[i + gap] == '.') {
            if (dotCount != 0)
              return false;
            dotCount++;
          } else if (pSelf->Value[i + gap] == 'E') {
            if (ECount != 0)
              return false;

            if (pSelf->Value[i + gap + 1] != '+' &&
                pSelf->Value[i + gap + 1] != '-')
              return false;
            ECount++;
          } else if (pSelf->Value[i + gap] == '+') {
            if (i + gap != start &&
                (i + gap != start && pSelf->Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(pSelf->Value[i + gap + 1]))
              return false;
          } else if (pSelf->Value[i + gap] == '-') {
            if (i + gap != start &&
                (i + gap != start && pSelf->Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(pSelf->Value[i + gap + 1]))
              return false;
          } else if (!__IsDecimal(pSelf->Value[i + gap]))
            break;
          gap++;
        }
        gap -= 1;
        break;
      default:
        Exception(ERROR, "지원하는 형식이 아닙니다. [ch:%%%C]", pFormat->Value[i]);
        break;
      }
      continue;
    }
    // clang-format on
    if (pSelf->Value[i + gap] != pFormat->Value[i])
      return false;
  }

  if (pSelf->Length != i + gap)
    return false;
  return true;
}
