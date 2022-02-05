
#include <CString.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Json.h>
#include <JsonAry.h>
#include <Private_Json.h>
#include <Private_JsonAry.h>
#include <StringAry.h>
#include <StringLib.h>

// 탐색할게 없으면 true
static bool
GetField(JSONObject pSelf,
         String pString,
         Index_t pStart,
         Index_t* out_pEndSymbolMark)
{
  if (pString->Length <= pStart)
    return true;

  // index를 기준으로 첫번째 "를 읽음
  Index_t i, StartSymbolMark = 0, EndSymbolMark = 0;
  char findCh = '\0';
  for (i = pStart; pString->Value[i] != '\0'; i++) {
    if (pString->Value[i] == '\"' || pString->Value[i] == '\'') {
      StartSymbolMark = i;

      if (pString->Value[i] == '\"')
        findCh = '\"';
      else
        findCh = '\'';

      Index_t j;
      for (j = i + 1; pString->Value[j] != '\0'; j++) {
        if (pString->Value[j] == '\\') {
          j++;
          continue;
        }

        // [*] 종료 조건
        if (pString->Value[j] == findCh) {
          EndSymbolMark = j;
          String ExtractValue = StringLibMethod.Extract(
            pString, StartSymbolMark + 1, EndSymbolMark);

          if (StringAryMethod.Contains(pSelf->m_FieldNames, ExtractValue)) {
            Exception(ERROR,
                      "같은 오브젝트 안에 똑같은 필드명이 여러 개 존재할 수 "
                      "없습니다. [field:%S]",
                      ExtractValue->Value);
            return true;
          }

          (*out_pEndSymbolMark) = EndSymbolMark;

          // 현재 JSONObject안에 넣음.
          StringAryMethod.Push(pSelf->m_FieldNames, ExtractValue);
          pSelf->m_FieldLength++;

          return false;
        }
      }
      // 필터
    } else if (IsSpace(pString->Value[i]) || pString->Value[i] == ',') {
      continue;
    } else {
      if (pString->Value[i] != '\0') {
        Exception(ERROR,
                  "JSON안에 이상한 문자열이 있습니다. [ch:%C,index:%u]",
                  pString->Value[i],
                  i);
        return true;
      }
    }
  }

  // [*] 필드명이 없는 경우 종료
  return true;
}

static bool
GetAry_Excute()
{
  return false;
}

static bool
GetAry(JSONObject pSelf,
       String pString,
       Index_t pStart,
       Index_t* out_pEndSymbolMark)
{
  // 대괄호 문자열을 추출한다음 그걸 재귀문으로 분석시키고 등록함

  Index_t BraceStackPointer = 1;
  Index_t i = pStart;
  while (pString->Value[i] != '\0') {
    i++;
    if (pString->Value[i] == '[') {
      BraceStackPointer++;
      continue;
    }

    if (pString->Value[i] == ']') {
      BraceStackPointer--;

      if (BraceStackPointer == 0) {
        String InputValue = StringLibMethod.Extract(pString, pStart, i + 1);
        JSONAry Ary = JSONAry_Constructor();
        Ary->m_Parent.m_Object = pSelf;
        Ary->m_Parent.IsObject = true;
        (*out_pEndSymbolMark) = i + 1;

        break;
      }
    }
  }
  return true;
}

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
      if (GetField(pSelf, _Value, index, &EndSymbolMark))
        break;
    }
    // [-] 현재 읽는 게 콘텐츠라면
    else {
      // 값을 저장할 노드를 생성함.
      JSONNode MakeNode = JSON_NodeCreate();
      MakeNode->m_Name = FieldName;
      MakeNode->Next = NULL;

      // 콜론 위치로 이동함.
      StartSymbolMark = StringMethod.IndexAt(_Value, String(":"), index);

      Index_t gap = 0, totalPosition = 0, BraceStackPointer = 0;
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
            // 시작점 설정
            i = StartSymbolMark + gap + 1;
            // 파싱
            while (_Value->Value[i] != '\"') {
              // 문자열이 닫히지 않은 경우
              if (_Value->Value[i] == '\n' || _Value->Value[i] == '\0') {
                Exception(ERROR, "문자열이 닫히지 않았습니다.");
                return false;
              }
              i++;
            }

            // 추출
            InputValue =
              StringLibMethod.Extract(_Value, StartSymbolMark + gap, i);

            //대입
            MakeNode->m_DataType = JSONDataType_String;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = i;

            break;
          case '\'':
            // 시작점 설정
            i = StartSymbolMark + gap + 1;
            // 파싱
            while (_Value->Value[i] != '\'') {
              // 문자열이 닫히지 않은 경우
              if (_Value->Value[i] == '\n' || _Value->Value[i] == '\0') {
                Exception(ERROR, "문자열이 닫히지 않았습니다.");
                return false;
              }
              i++;
            }

            // 추출
            InputValue =
              StringLibMethod.Extract(_Value, StartSymbolMark + gap, i);

            //대입
            MakeNode->m_DataType = JSONDataType_String;
            MakeNode->m_Value.StringValue = InputValue;
            EndSymbolMark = i;

            break;
          // [+] 참일 때
          case 't':
          case 'T':
            if (!IsSpace(_Value->Value[totalPosition + 5]) &&
                _Value->Value[totalPosition + 5] != ',' &&
                _Value->Value[totalPosition + 5] != '\0') {
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
                _Value->Value[totalPosition + 6] != ',' &&
                _Value->Value[totalPosition + 6] != '\0') {
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
            if (!IsSpace(_Value->Value[totalPosition + 5]) &&
                _Value->Value[totalPosition + 5] != ',' &&
                _Value->Value[totalPosition + 5] != '\0') {
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
            BraceStackPointer = 1;
            i = StartSymbolMark + gap;
            JSONObject Obj = JSON_Constructor();
            while (_Value->Value[i] != '\0') {
              i++;
              if (_Value->Value[i] == '{') {
                BraceStackPointer++;
                continue;
              }

              if (_Value->Value[i] == '}') {
                BraceStackPointer--;

                if (BraceStackPointer == 0) {
                  InputValue = StringLibMethod.Extract(
                    _Value, StartSymbolMark + gap, i + 1);

                  Obj->m_Parent = pSelf;
                  JSON_Read_Str(Obj, InputValue);
                  break;
                }
              }
            }

            if (BraceStackPointer != 0) {
              Exception(ERROR, "중괄호 구성이 잘못되어 있습니다.");
              return false;
            }

            MakeNode->m_DataType = JSONDataType_JSONObject;
            MakeNode->m_Value.ReferenceValue = Obj;
            EndSymbolMark = i;
            break;
          // [+] 배열일 때
          case '[':
            // [*] 무시하는 코드
            EndSymbolMark =
              StringMethod.IndexAt(_Value, String("]"), StartSymbolMark + gap);

            break;
          default:
            // [+] 숫자일 때
            if (IsDecimal(_Value->Value[StartSymbolMark + gap])) {
              i = StartSymbolMark + gap;
              while (_Value->Value[i] != ',' && !IsSpace(_Value->Value[i]) &&
                     _Value->Value[i] != '\0')
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

  return true;
}