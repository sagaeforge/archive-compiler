
#include <CString.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <StringAry.h>
#include <StringLib.h>

#define MAX_JSONOBJECT_CHILD_COUNT 32

bool
JSON_Read_Str(JSONObject pSelf, const String pString)
{
  String _Value = StringMethod.Trim(pString);

  // [*] 전제 조건 검사 및 읽기 최적화
  if (_Value->Value[0] != '{' || _Value->Value[_Value->Length - 1] != '}') {
    Exception(ERROR, "JSON 구조 형식이 아닙니다.");
    return false;
  }
  String Optimization = StringLibMethod.Extract(_Value, 1, _Value->Length - 1);
  StringMethod.Destructor(&_Value);
  _Value = StringMethod.Trim(Optimization);
  StringMethod.Destructor(&Optimization);

  // [*] 알고리즘
  bool NowTurnIsFieldName = true;
  String FieldName = NULL;
  Index_t index = 0, StartSymbolMark = 0, EndSymbolMark = 0, i = 0;
  while (true) {
    // [-] 현재 읽는 게 필드명이라면
    if (NowTurnIsFieldName) {
      // index를 기준으로 첫번째 "를 읽음
      StartSymbolMark = StringMethod.IndexAt(_Value, String("\""), index);

      // 필드명이 없는 경우 종료
      if (StartSymbolMark == -1)
        break;

      // Temp를 기준으로 두번째 "를 읽음
      EndSymbolMark =
        StringMethod.IndexAt(_Value, String("\""), StartSymbolMark + 1);

      // 필드명을 추출함.
      FieldName =
        StringLibMethod.Extract(_Value, StartSymbolMark + 1, EndSymbolMark);

      if (StringAryMethod.Contains(pSelf->m_FieldNames, FieldName)) {
        Exception(ERROR,
                  "똑같은 필드명이 2개 이상 존재할 수 없습니다. [field:%S]",
                  FieldName->Value);
      }

      // 현재 JSONObject안에 넣음.
      StringAryMethod.Push(pSelf->m_FieldNames, FieldName);
      pSelf->m_FieldLength++;
    }
    // [-] 현재 읽는 게 콘텐츠라면
    else {
      // 값을 저장할 노드를 생성함.
      JSONNode MakeNode = JSON_NodeCreate();
      MakeNode->m_Name = FieldName;
      MakeNode->m_Length = 1;
      MakeNode->Next = NULL;

      // 콜론 위치로 이동함.
      StartSymbolMark = StringMethod.IndexAt(_Value, String(":"), index);

      Index_t gap = 0, totalPosition = 0;
      while (true) {
        gap++;
        totalPosition = StartSymbolMark + gap;
        // 콜론 뒤에 공백이 있다면 무시할 수 있도록 gap을 만듬
        if (IsSpace(_Value->Value[totalPosition]))
          continue;

        // 판별
        String InputValue = NULL;
        switch (_Value->Value[totalPosition]) {
          // [+] 문자일 때
          case '\"':
            i = StartSymbolMark + gap + 1;
            while (_Value->Value[i] != '\"') {
              if (_Value->Value[i] == '\n') {
                Exception(ERROR, "문자열이 닫히지 않았습니다.");
                return false;
              }
              i++;
            }

            InputValue =
              StringLibMethod.Extract(_Value, StartSymbolMark + gap, i);

            MakeNode->m_DataType = JSONDataType_String;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = i;

            break;
          // [+] 참일 때
          case 't':
          case 'T':
            if (!IsSpace(_Value->Value[totalPosition + 5]) &&
                _Value->Value[totalPosition + 5] != ',') {
              Exception(ERROR, "알 수 없는 키워드입니다.");
              return false;
            }

            InputValue =
              StringLibMethod.Extract(_Value, totalPosition, totalPosition + 4);
            String TrueValue = StringMethod.ToUpper(InputValue);

            if (!StringMethod.Compare(TrueValue, String("TRUE"))) {
              Exception(
                ERROR, "알 수 없는 키워드입니다. [word:%S]", InputValue->Value);
              return false;
            }

            MakeNode->m_DataType = JSONDataType_Boolean;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = totalPosition + 4;
            break;
          // [+] 거짓일 때
          case 'f':
          case 'F':
            if (!IsSpace(_Value->Value[totalPosition + 6]) &&
                _Value->Value[totalPosition + 6] != ',') {
              Exception(ERROR, "알 수 없는 키워드입니다.");
              return false;
            }

            InputValue =
              StringLibMethod.Extract(_Value, totalPosition, totalPosition + 5);
            String FalseValue = StringMethod.ToUpper(InputValue);

            if (!StringMethod.Compare(FalseValue, String("FALSE"))) {
              Exception(
                ERROR, "알 수 없는 키워드입니다. [word:%S]", InputValue->Value);
              return false;
            }

            MakeNode->m_DataType = JSONDataType_Boolean;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = totalPosition + 5;
            break;
          // [+] 널일 때
          case 'n':
          case 'N':
            if (!IsSpace(_Value->Value[totalPosition + 4]) &&
                _Value->Value[totalPosition + 4] != ',') {
              Exception(ERROR, "알 수 없는 키워드입니다.");
              return false;
            }

            InputValue =
              StringLibMethod.Extract(_Value, totalPosition, totalPosition + 4);
            String nullValue = StringMethod.ToUpper(InputValue);

            if (!StringMethod.Compare(nullValue, String("NULL"))) {
              Exception(
                ERROR, "알 수 없는 키워드입니다. [word:%S]", InputValue->Value);
              return false;
            }

            MakeNode->m_DataType = JSONDataType_NULL;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = totalPosition + 4;
            break;
          // [+] 객체일 때
          case '{':
            // TODO 임시로 무시하도록 설계함.
            EndSymbolMark =
              StringMethod.IndexAt(_Value, String("}"), StartSymbolMark + gap);

            break;
          // [+] 배열일 때
          case '[':
            // TODO 임시로 무시하도록 설계함.
            EndSymbolMark =
              StringMethod.IndexAt(_Value, String("]"), StartSymbolMark + gap);
            break;
          default:
            // [+] 숫자일 때
            if (IsDecimal(_Value->Value[StartSymbolMark + gap])) {
              i = StartSymbolMark + gap;
              while (_Value->Value[i] != ',' && !IsSpace(_Value->Value[i]))
                i++;

              InputValue =
                StringLibMethod.Extract(_Value, StartSymbolMark + gap, i);
              if (!StringLibMethod.IsDigit(InputValue)) {
                Exception(
                  ERROR, "숫자가 아닙니다. [word:%S]", InputValue->Value);
                return false;
              }

              MakeNode->m_DataType = JSONDataType_Digit;
              MakeNode->m_Value.StringValue = InputValue;
              EndSymbolMark = i;
            } else {
              Exception(ERROR, "JSON 값 형식이 잘못되었습니다.");
              return false;
            }
        }
        break;
      }

      // 삽입점 찾기
      JSONNode InsertNode = pSelf->m_Nodes;
      if (InsertNode == NULL) {
        pSelf->m_Nodes = MakeNode;
      } else {
        while (InsertNode->Next != NULL)
          InsertNode = InsertNode->Next;
        InsertNode->Next = MakeNode;
      }
    }
    // [-] 공통적으로 해야하는 부분
    index = EndSymbolMark + 1;
    NowTurnIsFieldName = !NowTurnIsFieldName;
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
