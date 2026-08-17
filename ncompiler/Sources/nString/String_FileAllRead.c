
#include <stdio.h>

#include <Exception.h>
#include <Module/nString.h>
#include <Module/nStringAry.h>

#define MAXLINENUMS 65536

nStringAry_t*
String_FileAllRead(FILE* pFile)
{
  fseek(pFile, 0, SEEK_SET);

  nStringAry_ptr _Ary = nStringAry(0);
  wchar_t _temp[MAXLINENUMS];
  int _pos = 0;
  while (true) {
    int _in = fgetc(pFile);

    if (_in == '\n') {
      StringAry_Push(_Ary, nString(_temp));
      int _i = 0;
      for (_i = 0; _i < _pos; _i++)
        _temp[_i] = L'\0';

      _pos = 0;
      continue;
    }
    if (_in == EOF) {
      StringAry_Push(_Ary, nString(_temp));
      break;
    }

    _temp[_pos++] = (wchar_t)_in;

    if (_pos == MAXLINENUMS) {
      Exception(ERROR, "버퍼 공간을 다 사용했습니다. 파일 구조를 바꿔주세요.");
      StringAry_Destructor(&_Ary);
      return NULL;
    }
  }

  return _Ary;
}
