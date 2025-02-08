#include <locale>

#include <iostream>

// #include "00_app/lib/BaseException.hpp"
#include "00_app/repl/repl.h"
#include <unicode/unistr.h>

int main() {
  // 한글 출력을 위한 설정
  std::locale::global(std::locale("")); // 한국어 로케일 설정
  // auto exception = nugdev::compiler::lib::BaseException("test");
  auto str = icu::UnicodeString("test");

  int a;

  using namespace nugdev::compiler::repl;
  Repl{}.run();
  std::cin >> a;
  return 0;
}