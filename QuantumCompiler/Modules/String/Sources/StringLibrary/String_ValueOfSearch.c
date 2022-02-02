
#include <Exception.h>
#include <StringLib.h>
#include <string.h>

Func_t
String_ValueOfSearch(const char* DataType)
{
  if (strstr("bool", DataType))
    return (Func_t)&String_ValueOf_Bool;
  else if (strstr("short", DataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("int", DataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("long", DataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("longlong", DataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("unsigned short", DataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned int", DataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned long", DataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned long long", DataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("float", DataType))
    return (Func_t)&String_ValueOf_Digit;
  else if (strstr("double", DataType))
    return (Func_t)&String_ValueOf_Digit;
  else {
    Exception(ERROR, "지원하지 않는 형식입니다. [type:%s]", DataType);
    return NULL;
  }
}