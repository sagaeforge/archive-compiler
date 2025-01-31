#include <locale>

#include "00_app/repl/repl.h"

int main() {
  // 한글 출력을 위한 설정
  std::locale::global(std::locale("")); // 한국어 로케일 설정

  using namespace nugdev::compiler::repl;
  Repl{}.run();
  return 0;
}