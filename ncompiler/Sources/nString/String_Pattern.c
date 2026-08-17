
#include <Exception.h>
#include <Module/nString.h>

bool
String_Pattern(const nString_t* pSelf, const nString_t* pFormat)
{
  int i;
  int gap = 0;
  int dotCount = 0, ECount = 0;
  int start;
  for (i = 0; i < pFormat->m_Length; i++) {
    if (pFormat->m_Value[i] == '%') {
      i++;
      gap -= 1;
      // clang-format off
      switch (pFormat->m_Value[i]) {
      // '%'일 때
      case '%':
        if (pSelf->m_Value[i + gap] != '%')
          return false;
        break;
      // 문자일 때
      case 's': case 'S':
        if (pSelf->m_Value[i + gap] != '\"')
          return false;
        gap++;
        while (pSelf->m_Value[i + gap] != '\"' && pSelf->m_Value[i + gap] != '\0')
          gap++;

        if (pSelf->m_Value[i + gap] == '\0')
          return false;
        break;
      // 정수일 때
      case 'd': case 'D':
        while (__IsDecimal(pSelf->m_Value[i + gap]) && pSelf->m_Value[i + gap] != '\0')
          gap++;
        break;
      // 16진수일 때
      case 'x': case 'X':
        if (pSelf->m_Value[i + gap] != '0' && (pSelf->m_Value[i + gap + 1] != 'x' ||
                                            pSelf->m_Value[i + gap + 1] != 'X'))
          return false;
        gap += 2;
        while (__IsHex(pSelf->m_Value[i + gap]) && pSelf->m_Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 2진수일 때
      case 'b': case 'B':
        if (pSelf->m_Value[i + gap] != '0' && (pSelf->m_Value[i + gap + 1] != 'b' ||
                                            pSelf->m_Value[i + gap + 1] != 'B'))
          return false;
        gap += 2;
        while (__IsBinary(pSelf->m_Value[i + gap]) && pSelf->m_Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      // 8진수일 때
      case 'o': case 'O':
        if (pSelf->m_Value[i + gap] != '0' && (pSelf->m_Value[i + gap + 1] != 'o' ||
                                            pSelf->m_Value[i + gap + 1] != 'O'))
          return false;
        gap += 2;
        while (__IsOctal(pSelf->m_Value[i + gap]) && pSelf->m_Value[i + gap] != '\0')
          gap++;
        gap -= 1;
        break;
      case 'f': case 'F':
        start = i + gap;
        while (true) {
          if (pSelf->m_Value[i + gap] == '.') {
            if (dotCount != 0)
              return false;
            dotCount++;
          } else if (pSelf->m_Value[i + gap] == 'E') {
            if (ECount != 0)
              return false;

            if (pSelf->m_Value[i + gap + 1] != '+' &&
                pSelf->m_Value[i + gap + 1] != '-')
              return false;
            ECount++;
          } else if (pSelf->m_Value[i + gap] == '+') {
            if (i + gap != start &&
                (i + gap != start && pSelf->m_Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(pSelf->m_Value[i + gap + 1]))
              return false;
          } else if (pSelf->m_Value[i + gap] == '-') {
            if (i + gap != start &&
                (i + gap != start && pSelf->m_Value[i + gap - 1] != 'E'))
              return false;
            if (!__IsDecimal(pSelf->m_Value[i + gap + 1]))
              return false;
          } else if (!__IsDecimal(pSelf->m_Value[i + gap]))
            break;
          gap++;
        }
        gap -= 1;
        break;
      default:
        Exception(ERROR, "지원하는 형식이 아닙니다. [ch:%%%C]", pFormat->m_Value[i]);
        break;
      }
      continue;
    }
    // clang-format on
    if (pSelf->m_Value[i + gap] != pFormat->m_Value[i])
      return false;
  }

  if (pSelf->m_Length != i + gap)
    return false;
  return true;
}
