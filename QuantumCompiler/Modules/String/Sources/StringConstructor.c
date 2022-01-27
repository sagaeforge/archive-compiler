
#include <Chs.h>
#include <GarbageCollection.h>
#include <String.h>
#include <stdio.h>

static String
StringConstructor_None()
{
  String temp = (String)MemoryCreate(sizeof(String_t));
  if (temp == NULL) {
    // Warning("문자열 객체를 생성할 수 없습니다. (Size:%lu)", sizeof(String));
    // TODO Exception 처리
    return NULL;
  }
  temp->IsNone = true;
  temp->Length = 0;
  temp->Value = NULL;
  return temp;
}

String
StringConstructor_Chs(const char* Value)
{
  String temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)Value, 1);

  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, Value, 1, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}
String
StringConstructor_Wcs(const wchar_t* Value)
{
  String temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)Value, 4);
  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, Value, 4, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}
String
StringConstructor_Str(String_t Value)
{
  return StringConstructor_Wcs(Value.Value);
}
String
StringConstructor_Strp(String Value)
{
  return StringConstructor_Wcs(Value->Value);
}