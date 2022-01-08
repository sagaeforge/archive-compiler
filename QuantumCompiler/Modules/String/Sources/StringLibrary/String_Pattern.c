
#include "Chs.h"
#include "Private_StringLib.h"

#include <stdio.h>

bool String_Pattern(String *Self, String *Format) {
  int i;
  int gap = 0;
  int dotCount = 0, ECount = 0;
  int start;
  for (i = 0; i < Format->Length; i++) {
    if (Format->Value[i] == '%') {
      i++;
      gap -= 1;
      // clang-format off
      switch (Format->Value[i]) {
      // '%'일 때
      case '%':
        if (Self->Value[i + gap] != '%')
          return false;
        break;
      // 문자일 때
      case 's': case 'S':
        if (Self->Value[i + gap] != '\"')
          return false;
        gap++;
        while (Self->Value[i + gap] != '\"' && Self->Value[i + gap] != '\0')
          gap++;

        if (Self->Value[i + gap] == '\0')
          return false;
        break;
      // 정수일 때
      case 'd': case 'D':
        while (__IsDecimal(Self->Value[i + gap]) && Self->Value[i + gap] != '\0')
          gap++;
        break;
      // 16진수일 때
      case 'x': case 'X':
        if (Self->Value[i + gap] != '0' && (Self->Value[i + gap + 1] != 'x' ||
                                            Self->Value[i + gap + 1] != 'X'))
          return false;
        gap += 2;
        while (__IsHex(Self->Value[i + gap]) && Self->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 2진수일 때
      case 'b': case 'B':
        if (Self->Value[i + gap] != '0' && (Self->Value[i + gap + 1] != 'b' ||
                                            Self->Value[i + gap + 1] != 'B'))
          return false;
        gap += 2;
        while (__IsBinary(Self->Value[i + gap]) && Self->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 8진수일 때
      case 'o':
      case 'O':
        if (Self->Value[i + gap] != '0' && (Self->Value[i + gap + 1] != 'o' ||
                                            Self->Value[i + gap + 1] != 'O'))
          return false;
        gap += 2;
        while (__IsOctal(Self->Value[i + gap]) && Self->Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      case 'f': case 'F':
        start = i + gap;
        while (true) {
          if (Self->Value[i + gap] == '.') {
            if (dotCount != 0)
              return false;
            dotCount++;
          } else if (Self->Value[i + gap] == 'E') {
            if (ECount != 0)
              return false;

            if (Self->Value[i + gap + 1] != '+' &&
                Self->Value[i + gap + 1] != '-')
              return false;
            ECount++;
          } else if (Self->Value[i + gap] == '+') {
            if (i + gap != start &&
                (i + gap != start && Self->Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(Self->Value[i + gap + 1]))
              return false;
          } else if (Self->Value[i + gap] == '-') {
            if (i + gap != start &&
                (i + gap != start && Self->Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(Self->Value[i + gap + 1]))
              return false;
          } else if (!__IsDecimal(Self->Value[i + gap]))
            break;
          gap++;
        }
        gap -= 1;
        break;
      default:
        printf("오류");
        break;
      }
      continue;
    }
    // clang-format on
    if (Self->Value[i + gap] != Format->Value[i])
      return false;
  }

  if (Self->Length != i + gap)
    return false;
  return true;
}
