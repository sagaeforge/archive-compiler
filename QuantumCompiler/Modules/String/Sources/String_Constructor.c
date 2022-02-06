
#include <Chs.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <String.h>

#include <stdio.h>

static String
String_Constructor_None()
{
  String temp = (String)MemoryCreate(sizeof(String_t));
  if (temp == NULL) {
    Exception(
      ERROR, "문자열 객체를 생성할 수 없습니다. [size:%lu]", sizeof(String_t));
    return NULL;
  }
  temp->m_IsNone = true;
  temp->m_Length = 0;
  temp->m_Value = NULL;
  return temp;
}

String
String_Constructor_Chs(const char* pValue)
{
  String temp = String_Constructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)pValue, 1);

  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, pValue, 1, len);
    temp->m_IsNone = false;
    temp->m_Length = len;
    temp->m_Value = tempValue;
  }

  return temp;
}

String
String_Constructor_Wcs(const wchar_t* pValue)
{
  String temp = String_Constructor_None();
  if (temp == NULL)
    return NULL;

  Length_t len = __StrLen((void*)pValue, 4);
  if (len != 0) {
    wcs tempValue = __WcsCreate(len);
    __StrSet(tempValue, pValue, 4, len);
    temp->m_IsNone = false;
    temp->m_Length = len;
    temp->m_Value = tempValue;
  }

  return temp;
}

String
String_Constructor_Str(String_t pValue)
{
  return String_Constructor_Wcs(pValue.m_Value);
}

String
String_Constructor_Strp(String pValue)
{
  return String_Constructor_Wcs(pValue->m_Value);
}