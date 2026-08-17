
#include <stdlib.h>

#include <Module/nString.h>

#pragma region 보이어 - 무어 알고리즘
// TODO 추후에 최적화

static void
preprocess_strong_suffix(int* shift, int* bpos, Wcs_t pat, int m)
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
preprocess_case2(int* shift, int* bpos, Wcs_t pat, int m)
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

#pragma endregion

Index_t
String_LastOfIndex(const nString_t* pSelf, const nString_t* pKeyWord)
{
  int c = 0;
  int s = 0, j;
  int m = pSelf->m_Length;
  int n = pKeyWord->m_Length;

  int* bpos = calloc(sizeof(int), m);
  int* shift = calloc(sizeof(int), m);

  for (int i = 0; i < m + 1; i++)
    shift[i] = 0;

  preprocess_strong_suffix(shift, bpos, pKeyWord->m_Value, m);
  preprocess_case2(shift, bpos, pKeyWord->m_Value, m);

  while (s <= n - m) {
    j = m - 1;

    while (j >= 0 && pKeyWord->m_Value[j] == pSelf->m_Value[s + j])
      j--;

    if (j < 0) {
      c = s;
      s += shift[0];
    } else
      s += shift[j + 1];
  }
  return c;
}
