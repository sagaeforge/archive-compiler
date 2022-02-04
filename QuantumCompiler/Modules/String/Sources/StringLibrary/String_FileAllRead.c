
#include <Chs.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>

#define MAXLINE 2048

StringAry
String_FileAllRead(FILE* pfile)
{
  fseek(pfile, 0, SEEK_SET);

  StringAry ary = StringAry(0);

  wcs temp = __WcsCreate(MAXLINE);
  int pos = 0;
  while (true) {
    int in = fgetc(pfile);
    if (in == '\n') {
      StringAry_Push(ary, String(temp));
      int i = 0;
      while (i < pos) {
        temp[i] = '\0';
        i++;
      }

      pos = 0;
      continue;
    }
    if (in == EOF) {
      StringAry_Push(ary, String(temp));
      break;
    }

    temp[pos++] = in;

    if (pos == MAXLINE) {
      Exception(ERROR, "버퍼 공간을 다 사용했습니다. 파일 구조를 바꿔주세요.");
      MemoryRemove(temp);
      StringAry_Destructor(&ary);
      return NULL;
    }
  }

  MemoryRemove(temp);
  return ary;
}