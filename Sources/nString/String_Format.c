
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>
#include <Module/nStringAry.h>

nString_t*
String_Format(const nString_t* pFormat, ...)
{
  va_list ap;
  va_start(ap, pFormat);
  int percent = String_Count(pFormat, nString("%"));
  int perper = String_Count(pFormat, nString("%%"));
  int ptr_param;

  nStringAry_ptr Ary = nStringAry(0);

  int i;
  for (i = 0; i < percent; i++) {
    ptr_param = String_IndexFor(pFormat, nString("%"), i);
    char ch[2];
    int64_t temp_d;
    double temp_f;
    // clang-format off
    switch (pFormat->m_Value[ptr_param + 1]) {
    case 's': StringAry_Push(Ary, va_arg(ap, nString_ptr)); break;
    case 'S': StringAry_Push(Ary, nString(va_arg(ap, char *))); break;
    case 'w': case 'W': StringAry_Push(Ary, nString(va_arg(ap, wchar_t *))); break;
    case 'c': case 'C': ch[0] = va_arg(ap, int);
                        ch[1] = '\0';
                        StringAry_Push(Ary, nString(ch)); 
                        break;
    case 'd': case 'D': temp_d = va_arg(ap, int);
                        StringAry_Push(Ary, toString((int64_t) temp_d)); 
                        break;
    case 'l': case 'L': temp_d = va_arg(ap, int64_t);
                        StringAry_Push(Ary, toString((int64_t) temp_d)); 
                        break;
    case 'g': case 'G': 
    case 'f': case 'F': temp_f = va_arg(ap, double);
                        StringAry_Push(Ary, toString(temp_f, 16)); 
                        break;
    case '%': if(pFormat->m_Value[ptr_param - 1] != '%')
              {
                ch[0] = '%'; ch[1] = '\0';
                StringAry_Push(Ary, nString(ch));
              }
              break;
    default:
        Exception(ERROR, "지원하는 형식이 아닙니다. [ch:%%%C]", pFormat->m_Value[ptr_param + 1]);
      break;
    }
    // clang-format on
  }

  Length_t Leng = 0;
  for (i = 0; i < Ary->m_Length; i++)
    Leng += StringAry_get(Ary, i)->m_Length;

  Leng += pFormat->m_Length - ((percent - perper) * 2);
  ptr_param = 0;

  int gap = 0;
  Wcs_t temp = __WCSMAKE(Leng);
  for (i = 0; i < pFormat->m_Length; i++) {
    if (pFormat->m_Value[i] == '%') {
      i++;
      gap -= 1;

      nString_ptr str;
      // clang-format off
      switch (pFormat->m_Value[i]) {
        case 's': case 'S': case 'w': case 'W': case 'c': 
        case 'C': case 'd': case 'D': case 'l': case 'L': 
        case 'f': case 'F': case '%': case 'g': case 'G':
          str = StringAry_get(Ary, ptr_param++);
          Index_t _i;
          for (_i =  i + gap; _i < str->m_Length; _i++)
            temp[_i] = str->m_Value[_i];
          gap += str->m_Length - 1;
          break;
        default:
          break;
      }
      // clang-format on
      continue;
    }

    temp[i + gap] = pFormat->m_Value[i];
  }
  va_end(ap);
  return nString(temp);
}
