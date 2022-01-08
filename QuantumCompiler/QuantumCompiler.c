
#include "Private_StringLib.h"
#include "ProgramManager.h"
#include "String.h"
#include "StringLib.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
  ProgramManager_Init();
  Application.ProgramStart();

  // clang-format off
  // bool isF = 
  // String_Pattern(
  //   String(L"누군가: 0.123E+1 123"),
  //   String(L"누군가: %f 123")
  //   );
  // String *bab1 = String_Notatiosn(244, 2);
  // clang-format on
  // printf("%S", bab1->Value);
  // printf("%d", isF);
  // bool isc = String_IsDigit(String("12-s34.23"));
  // printf("%d", isc);
  // printf("")
  String_Format(String("%s%s"), String("2131234"), String("2131234"));

  Application.ProgramQuit();
  return 0;
}
