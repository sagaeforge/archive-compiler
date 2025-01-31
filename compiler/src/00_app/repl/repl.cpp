#include "00_app/repl/repl.h"

#include <iostream>
#include <string>

#include <unicode/unistr.h>

// UTF-8에서 wstring(UTF-16)으로 변환
std::wstring utf8_to_wstring(const std::string &str) {
  // UTF-8 문자열을 ICU UnicodeString으로 변환
  icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(str);

  // UnicodeString을 wstring으로 변환
  std::wstring result;
  result.resize(ustr.length());
  ustr.extract(0, ustr.length(), &result[0]);

  return result;
}

namespace nugdev::compiler::repl {

void Repl::run() {
  icu::UnicodeString icu_str = icu::UnicodeString::fromUTF8("Hello, World!");

  while (true) {
    std::cout << ">> ";

    std::string input;
    std::getline(std::cin, input);
    if (input == "exit") {
      break;
    }

    std::wstring winput = utf8_to_wstring(input);
    std::wcout << L"입력: " << winput << std::endl;
  }
}

} // namespace nugdev::compiler::repl