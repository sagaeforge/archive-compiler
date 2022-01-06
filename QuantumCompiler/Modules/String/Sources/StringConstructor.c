
#include "Chs.h"
#include "GarbageCollection.h"
#include "String.h"
#include <stdio.h>

static String *StringConstructor_None() {
  String *temp = (String *)MemoryCreate(sizeof(String));
  if (temp == NULL) {
    // Warning("문자열 객체를 생성할 수 없습니다. (Size:%lu)", sizeof(String));
    // TODO Exception 처리
    return NULL;
  }
  temp->IsConst = false;
  temp->IsNone = true;
  temp->Length = 0;
  temp->Value = NULL;
  return temp;
}

String *StringConstructor_Chs(const char *Value) {
  String *temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length len = __Chslen(Value);
  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __WcsChsSet(tempValue, Value, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}
String *StringConstructor_Wcs(const wchar_t *Value) {
  String *temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length len = __Wcslen(Value);
  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __WcsWcsSet(tempValue, Value, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}
String *StringConstructor_Str(String Value) {
  return StringConstructor_Wcs(Value.Value);
}
String *StringConstructor_Strp(String *Value) {
  return StringConstructor_Wcs(Value->Value);
}