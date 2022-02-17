
#include <Module/nString.h>
#include <Module/nStringAry.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#pragma region 디버깅용
Func_t Debugging[] = { (Func_t)String_Constructor_Chs,
                       (Func_t)String_Constructor_Wcs,
                       (Func_t)String_Constructor_Str,
                       (Func_t)String_Constructor_Strp,
                       (Func_t)String_Destructor,
                       (Func_t)String_Join,
                       (Func_t)String_Append,
                       (Func_t)String_SubString,
                       (Func_t)String_Loop,
                       (Func_t)String_Split,
                       (Func_t)String_Compare,
                       (Func_t)String_Trim,
                       (Func_t)String_Contains,
                       (Func_t)String_Count,
                       (Func_t)String_Get,
                       (Func_t)String_Set,
                       (Func_t)String_Length,
                       (Func_t)String_ToLower,
                       (Func_t)String_ToUpper,
                       (Func_t)String_IndexOf,
                       (Func_t)String_IndexAt,
                       (Func_t)String_IndexFor,
                       (Func_t)String_LastOfIndex,
                       (Func_t)String_Replace,
                       (Func_t)String_ReplaceAt,
                       (Func_t)String_ReplaceAll,
                       (Func_t)String_Left,
                       (Func_t)String_Right,
                       (Func_t)String_Middle,
                       (Func_t)String_Extract,
                       (Func_t)String_Reverse,
                       (Func_t)String_Search,
                       (Func_t)String_IsAlpha,
                       (Func_t)String_IsLower,
                       (Func_t)String_IsUpper,
                       (Func_t)String_IsDecimal,
                       (Func_t)String_IsDigit,
                       (Func_t)String_IsSpace,
                       (Func_t)String_IsAlphaDigit,
                       (Func_t)String_IsHex,
                       (Func_t)String_IsControl,
                       (Func_t)String_IsOctal,
                       (Func_t)String_IsBinary,
                       (Func_t)String_Format,
                       (Func_t)String_Pattern,
                       (Func_t)String_Check,
                       (Func_t)String_Notation,
                       (Func_t)String_FileAllRead,
                       (Func_t)String_FileAllWrite,
                       (Func_t)String_Print,
                       (Func_t)String_PrintErr,
                       (Func_t)String_PrintLine,
                       (Func_t)String_ToString_Bool,
                       (Func_t)String_ToString_Decimal,
                       (Func_t)String_ToString_Decimal_Unsigned,
                       (Func_t)String_ToString_Digit,
                       (Func_t)String_ToString_StringAry,
                       (Func_t)String_ValueOf_Bool,
                       (Func_t)String_ValueOf_Decimal,
                       (Func_t)String_ValueOf_Decimal_Unsigned,
                       (Func_t)String_ValueOf_Digit,
                       (Func_t)StringAry_Constructor,
                       (Func_t)StringAry_Destructor,
                       (Func_t)StringAry_get,
                       (Func_t)StringAry_Insert,
                       (Func_t)StringAry_Remove,
                       (Func_t)StringAry_Push,
                       (Func_t)StringAry_Pop,
                       (Func_t)StringAry_Search,
                       (Func_t)StringAry_Contains,
                       (Func_t)StringAry_toAry,
                       (Func_t)StringAry_toList,
                       (Func_t)StringAry_CountIf };
#pragma endregion

static void
preprocess_strong_suffix(int* shift, int* bpos, char* pat, int m)
{
  int i = m, j = m + 1;
  bpos[i] = j;
  while (i > 0) {
    while (j <= m && pat[i - 1] != pat[j - 1]) {
      if (shift[j] == 0)
        shift[j] = j - i;
      j = bpos[j];
    }
    i--;
    j--;
    bpos[i] = j;
  }
}

static void
preprocess_case2(int* shift, int* bpos, char* pat, int m)
{
  int i, j;
  j = bpos[0];
  for (i = 0; i <= m; i++) {
    if (shift[i] == 0)
      shift[i] = j;

    if (i == j)
      j = bpos[j];
  }
}

void
search(char* text, char* pat)
{
  // s is shift of the pattern with respect to text
  int s = 0, j;
  int m = __STRLEN(pat);
  int n = __STRLEN(text);

  int* bpos = calloc(sizeof(int), m);
  int* shift = calloc(sizeof(int), m);

  // initialize all occurrence of shift to 0
  for (int i = 0; i < m + 1; i++)
    shift[i] = 0;

  // do preprocessing
  preprocess_strong_suffix(shift, bpos, pat, m);
  preprocess_case2(shift, bpos, pat, m);

  while (s <= n - m) {
    j = m - 1;

    /* Keep reducing index j of pattern while characters of
         pattern and text are matching at this shift s*/
    while (j >= 0 && pat[j] == text[s + j])
      j--;

    /* If the pattern is present at the current shift, then index j
         will become -1 after the above loop */
    if (j < 0) {
      printf("pattern occurs at shift = %d\n", s);
      s += shift[0];
      return;
    } else
      /*pat[i] != pat[s+j] so shift the pattern
        shift[j+1] times  */
      s += shift[j + 1];
  }
  printf("Not Found ㅠㅠ");
}

int
main(int argc, char const* argv[])
{
  setlocale(LC_ALL, "");

  search("abcedefghij", "bce");

  return 0;
}
