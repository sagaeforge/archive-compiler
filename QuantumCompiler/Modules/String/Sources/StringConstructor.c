
#include <Chs.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <String.h>

#include <stdio.h>

static String
StringConstructor_None()
{
  String temp = (String)MemoryCreate(sizeof(String_t));
  if (temp == NULL) {
    Exception(
      ERROR, "문자열 객체를 생성할 수 없습니다. [size:%lu]", sizeof(String_t));
    return NULL;
  }
  temp->IsNone = true;
  temp->Length = 0;
  temp->Value = NULL;
  return temp;
}

String
StringConstructor_Chs(const char* pValue)
{
  String temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)pValue, 1);

  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, pValue, 1, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}

String
StringConstructor_Wcs(const wchar_t* pValue)
{
  String temp = StringConstructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)pValue, 4);
  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, pValue, 4, len);
    temp->IsNone = false;
    temp->Length = len;
    temp->Value = tempValue;
  }

  return temp;
}

String
StringConstructor_Str(String_t pValue)
{
  return StringConstructor_Wcs(pValue.Value);
}

String
StringConstructor_Strp(String pValue)
{
  return StringConstructor_Wcs(pValue->Value);
}