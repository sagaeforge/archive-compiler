
#include <stdarg.h>
#include <stdio.h>

#include <Chs.h>
#include <Exception.h>
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>

String
String_Format(String pFormat, ...)
{
  va_list ap;
  va_start(ap, pFormat);
  int percent = String_Count(pFormat, String("%"));
  int perper = String_Count(pFormat, String("%%"));
  int ptr_param;

  StringAry Ary = StringAryConstructor(0);

  int i;
  for (i = 0; i < percent; i++) {
    ptr_param = String_IndexFor(pFormat, String("%"), i);
    char ch[2];
    int64_t temp_d;
    double temp_f;
    // clang-format off
    switch (pFormat->Value[ptr_param + 1]) {
    case 's': StringAry_Push(Ary, va_arg(ap, String)); break;
    case 'S': StringAry_Push(Ary, String(va_arg(ap, char *))); break;
    case 'w': case 'W': StringAry_Push(Ary, String(va_arg(ap, wchar_t *))); break;
    case 'c': case 'C': ch[0] = va_arg(ap, int);
                        ch[1] = '\0';
                        StringAry_Push(Ary, String(ch)); 
                        break;
    case 'd': case 'D': temp_d = va_arg(ap, int);
                        StringAry_Push(Ary, toString((int64_t) temp_d)); 
                        break;
    case 'l': case 'L': temp_d = va_arg(ap, int64_t);
                        StringAry_Push(Ary, toString((int64_t) temp_d)); 
                        break;
    case 'f': case 'F': temp_f = va_arg(ap, double);
                        StringAry_Push(Ary, toString(temp_f)); 
                        break;
    case 'g': case 'G': temp_f = va_arg(ap, double);
                        StringAry_Push(Ary, String_Prettier(temp_f)); 
                        break;
    case '%': if(pFormat->Value[ptr_param - 1] != '%')
              {
                ch[0] = '%'; ch[1] = '\0';
                StringAry_Push(Ary, String(ch));
              }
              break;
    default:
        Exception(ERROR, "지원하는 형식이 아닙니다. [ch:%%%C]", pFormat->Value[ptr_param + 1]);
      break;
    }
    // clang-format on
  }

  Length_t Leng = 0;
  for (i = 0; i < Ary->Length; i++)
    Leng += StringAry_Get(Ary, i)->Length;

  Leng += pFormat->Length - ((percent - perper) * 2);
  ptr_param = 0;

  int gap = 0;
  wcs temp = __WcsCreate(Leng);
  for (i = 0; i < pFormat->Length; i++) {
    if (pFormat->Value[i] == '%') {
      i++;
      gap -= 1;

      String str;
      // clang-format off
      switch (pFormat->Value[i]) {
        case 's': case 'S': case 'w': case 'W': case 'c': 
        case 'C': case 'd': case 'D': case 'l': case 'L': 
        case 'f': case 'F': case '%': case 'g': case 'G':
          str = StringAry_Get(Ary, ptr_param++);
          __WcsWcsInsert(temp, str->Value, i + gap, str->Length);
          gap += str->Length - 1;
          break;
        default:
          break;
      }
      // clang-format on
      continue;
    }

    temp[i + gap] = pFormat->Value[i];
  }
  va_end(ap);
  return String(temp);
}
