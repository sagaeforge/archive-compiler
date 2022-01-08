
#include "Chs.h"
#include "Private_String.h"
#include "Private_StringLib.h"

#include <stdarg.h>

// TODO 구현
String *String_Format(String *Format, ...) {
  va_list ap;
  va_start(ap, Format);
  int percent = String_Count(Format, String("%"));
  int perper = String_Count(Format, String("%%"));
  int ParamLen = percent - perper;
  int Totallen = Format->Length - ParamLen;

  // String 객체 생성
  int i;
  for (i = 0; i < ParamLen; i++) {

    int ptr_param = String_IndexFor(Format, String("%"), i);
    String *str1;
    wcs str_wcs;
    chs str_chs;
    int Var;
    long long longVar;
    double DoubleVar;

    // clang-format off
    switch (Format->Value[ptr_param + 1]) {
    case 's':
      str1 = va_arg(ap, String *);
      Totallen += str1->Length;
      break;
    case 'S':
      str_chs = va_arg(ap, char *);
      Totallen += __Chslen(str_chs);\
      break;
    case 'w': case 'W':
      str_wcs = va_arg(ap, wchar_t *);
      Totallen += __Wcslen(str_wcs);
      break;
    case 'c': case 'C': Totallen++; break;
    case 'd': case 'D':
      Var = va_arg(ap, int);
      str1 = String_ToString_Decimal(Var);
      Totallen += str1->Length;
      break;
    case 'l': case 'L':
      longVar = va_arg(ap, long long);
      str1 = String_ToString_Decimal(longVar);
      Totallen += str1->Length;
      break;
    case 'f': case 'F':
      DoubleVar = va_arg(ap, double);
      str1 = String_ToString_Decimal(DoubleVar);
      Totallen += str1->Length;
      break;
    default:
      // Exception 처리
      // 지정된 포멧외 문자를 사용하였습니다.
      break;
    }
    // clang-format on
  }

  // String 객체에 값 대입

  // 새로운 객체 생성
  va_end(ap);
  return String("");
}
