
#include <Exception.h>
#include <StringLib.h>
#include <string.h>

Func_t
String_ValueOfSearch(const char* pDataType)
{
  if (strstr("bool", pDataType))
    return (Func_t)&String_ValueOf_Bool;
  else if (strstr("short", pDataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("int", pDataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("long", pDataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("longlong", pDataType))
    return (Func_t)&String_ValueOf_Decimal;
  else if (strstr("unsigned short", pDataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned int", pDataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned long", pDataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("unsigned long long", pDataType))
    return (Func_t)&String_ValueOf_Decimal_Unsigned;
  else if (strstr("float", pDataType))
    return (Func_t)&String_ValueOf_Digit;
  else if (strstr("double", pDataType))
    return (Func_t)&String_ValueOf_Digit;
  else {
    Exception(ERROR, "지원하지 않는 형식입니다. [type:%s]", pDataType);
    return NULL;
  }
}