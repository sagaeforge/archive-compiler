
#include <locale.h>
#include <nString.h>
#include <stdio.h>

Length_t nChs_WcsLen(const Wcs_t pValue) { return 4; }

int main(int argc, char const *argv[]) {
  setlocale(LC_ALL, "");
  nString_t Value2 = String(L"시발");
  printf("%S", Value2.m_Value);

  return 0;
}
