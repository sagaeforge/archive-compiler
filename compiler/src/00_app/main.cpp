#include <locale>

#include <iostream>

#include "00_app/repl/repl.h"

int main() {
  // 한글 출력을 위한 설정
  std::locale::global(std::locale("")); // 한국어 로케일 설정

  int a;

  using namespace nugdev::compiler::repl;
  Repl{}.run();
  std::cin >> a;
  return 0;
}