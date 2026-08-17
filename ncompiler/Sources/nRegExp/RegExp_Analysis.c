
#include <Module/nString.h>

// clang-format off

// clang-format on

nRegExpResult_t*
RegExp_Analysis(const nString_t* pSelf, const nRegExp_t* pRegExp)
{
  Index_t _i, _RegExpPointer = 0;
  for (_i = 0; _i < pSelf->m_Length; _i++) {

    // RegExp 분석
    while (true) {
      switch (pRegExp->m_Pattern->m_Value[_RegExpPointer]) {
        case '^':
          break;
        case '$':
          break;
        case '.':
          break;
        case '[':
          break;
        case '-':
          break;
        case '(':
          break;
        case '*':
          break;
        case '+':
          break;
        case '?':
          break;
        case '{':
          break;
        case '\\':
          switch (pRegExp->m_Pattern->m_Value[_RegExpPointer + 1]) {
            case 'w':
              break;
            case 'W':
              break;
            case 'd':
              break;
            case 'D':
              break;
            case 'b':
              break;
            case 'B':
              break;
            case 'A':
              break;
            case 'Z':
              break;
            case '^':
              break;
            case '$':
              break;
            case '.':
              break;
            case '[':
              break;
            case '-':
              break;
            case '(':
              break;
            case '*':
              break;
            case '+':
              break;
            case '?':
              break;
            case '{':
              break;
            case '\\':
              break;
            case '/':
              break;

            default:
              break;
          }
          break;
        default:
          break;
      }
    }
  }

  return NULL;
}