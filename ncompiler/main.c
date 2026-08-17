
#include <Module/nDigitAry.h>
#include <Module/nString.h>
#include <Module/nStringAry.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

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
