cmake_minimum_required(VERSION 3.0.0)
project(nCompiler VERSION 0.1.0)

include(CTest)
enable_testing()

set (nString_SRCS
  Sources/nString/String_Append.c
  Sources/nString/String_Check.c
  Sources/nString/String_Compare.c
  Sources/nString/String_Constructor_Chs.c
  Sources/nString/String_Constructor_Str.c
  Sources/nString/String_Constructor_Strp.c
  Sources/nString/String_Constructor_Wcs.c
  Sources/nString/String_Contains.c
  Sources/nString/String_Count.c
  Sources/nString/String_Destructor.c
  Sources/nString/String_Extract.c
  Sources/nString/String_FileAllRead.c
  Sources/nString/String_FileAllWrite.c
  Sources/nString/String_Format.c
  Sources/nString/String_Get.c
  Sources/nString/String_IndexAt.c
  Sources/nString/String_IndexFor.c
  Sources/nString/String_IndexOf.c
  Sources/nString/String_IsAlpha.c
  Sources/nString/String_IsAlphaDigit.c
  Sources/nString/String_IsBinary.c
  Sources/nString/String_IsControl.c
  Sources/nString/String_IsDecimal.c
  Sources/nString/String_IsDigit.c
  Sources/nString/String_IsHex.c
  Sources/nString/String_IsLower.c
  Sources/nString/String_IsOctal.c
  Sources/nString/String_IsSpace.c
  Sources/nString/String_IsUpper.c
  Sources/nString/String_Join.c
  Sources/nString/String_LastOfIndex.c
  Sources/nString/String_Left.c
  Sources/nString/String_Length.c
  Sources/nString/String_Loop.c
  Sources/nString/String_Middle.c
  Sources/nString/String_Notation.c
  Sources/nString/String_Pattern.c
  Sources/nString/String_Print.c
  Sources/nString/String_PrintErr.c
  Sources/nString/String_PrintLine.c
  Sources/nString/String_ReplaceAll.c
  Sources/nString/String_Replace.c
  Sources/nString/String_ReplaceFor.c
  Sources/nString/String_Reverse.c
  Sources/nString/String_Right.c
  Sources/nString/String_Search.c
  Sources/nString/String_Set.c
  Sources/nString/String_Split.c
  Sources/nString/String_SubString.c
  Sources/nString/String_ToLower.c
  Sources/nString/String_ToString_Bool.c
  Sources/nString/String_ToString_Decimal.c
  Sources/nString/String_ToString_Decimal_Unsigned.c
  Sources/nString/String_ToString_Digit.c
  Sources/nString/String_ToString_StringAry.c
  Sources/nString/String_ToUpper.c
  Sources/nString/String_Trim.c
  Sources/nString/String_ValueOf_Bool.c
  Sources/nString/String_ValueOf_Decimal.c
  Sources/nString/String_ValueOf_Decimal_Unsigned.c
  Sources/nString/String_ValueOf_Digit.c

  Sources/nString/StringAry_Constructor.c
  Sources/nString/StringAry_Contains.c
  Sources/nString/StringAry_CountIf.c
  Sources/nString/StringAry_Destructor.c
  Sources/nString/StringAry_Foreach.c
  Sources/nString/StringAry_Get.c
  Sources/nString/StringAry_Insert.c
  Sources/nString/StringAry_isAry.c
  Sources/nString/StringAry_isList.c
  Sources/nString/StringAry_Pop.c
  Sources/nString/StringAry_Push.c
  Sources/nString/StringAry_Remove.c
  Sources/nString/StringAry_Search.c
  Sources/nString/StringAry_toAry.c
  Sources/nString/StringAry_toList.c
)

add_executable(nCompiler
  main.c

  ${nString_SRCS}
)

target_include_directories(
  nCompiler

PUBLIC
  Includes/
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
