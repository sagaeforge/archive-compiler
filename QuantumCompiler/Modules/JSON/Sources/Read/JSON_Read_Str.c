
#include <CString.h>
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <StringAry.h>
#include <StringLib.h>

bool
JSON_Read_Str(JSONObject pSelf, const String pString)
{
  String _Value = StringMethod.Trim(pString);
  if (_Value->Value[0] != '{') {
    // Exception 처리
    return false;
  }
  if (_Value->Value[_Value->Length - 1] != '}') {
    // Exception 처리
    return false;
  }

  /*
    기초는 하기 쉬움
    String =>
    Digit =>
    Bool =>
    NULL =>

    근데 아래는 진짜 어려움
    Object =>
    Ary =>

    일단 기본 로직은 필드명을 추출하고 간단한거 먼저 구현함.

    그리고 배열은 배열을 지원할 수 있도록 구성하고 싶음.


  */

  return true;
}
