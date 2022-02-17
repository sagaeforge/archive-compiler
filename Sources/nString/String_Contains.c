
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

bool
String_Contains(const nString_t* pSelf, const nString_t* pValue)
{
  int s = 0, j;
  int m = pSelf->m_Length;
  int n = pValue->m_Length;

  int* bpos = calloc(sizeof(int), m);
  int* shift = calloc(sizeof(int), m);

  for (int i = 0; i < m + 1; i++)
    shift[i] = 0;

  preprocess_strong_suffix(shift, bpos, pValue->m_Value, m);
  preprocess_case2(shift, bpos, pValue->m_Value, m);

  while (s <= n - m) {
    j = m - 1;

    while (j >= 0 && pValue->m_Value[j] == pSelf->m_Value[s + j])
      j--;

    if (j < 0)
      return true;
    else
      s += shift[j + 1];
  }
  return false;
}
