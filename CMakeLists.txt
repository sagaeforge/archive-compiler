cmake_minimum_required(VERSION 3.0.0)
project(nCompiler VERSION 0.1.0)

include(CTest)
enable_testing()

set (nString_SRCS
  Sources/nString/String_Constructor_Chs.c
  Sources/nString/String_Contains.c
  Sources/nString/String_FileAllRead.c
  Sources/nString/String_IndexAt.c
  Sources/nString/String_IsAlphaDigit.c
  Sources/nString/String_IsDigit.c
  Sources/nString/String_IsSpace.c
  Sources/nString/String_Left.c
  Sources/nString/String_Notation.c
  Sources/nString/String_Reverse.c
  Sources/nString/String_Split.c
  Sources/nString/String_Trim.c          
  Sources/nString/String_Append.c
  Sources/nString/String_Constructor_Str.c
  Sources/nString/String_Count.c
  Sources/nString/String_FileAllWrite.c
  Sources/nString/String_IndexFor.c
  Sources/nString/String_IsBinary.c
  Sources/nString/String_IsHex.c
  Sources/nString/String_IsUpper.c
  Sources/nString/String_Length.c
  Sources/nString/String_Pattern.c
  Sources/nString/String_ReplaceAll.c
  Sources/nString/String_Right.c
  Sources/nString/String_SubString.c
  Sources/nString/String_UTF8Decorder.c
  Sources/nString/String_Check.c
  Sources/nString/String_Constructor_Strp.c
  Sources/nString/String_Destructor.c
  Sources/nString/String_Format.c
  Sources/nString/String_IndexOf.c
  Sources/nString/String_IsControl.c
  Sources/nString/String_IsLower.c
  Sources/nString/String_Join.c
  Sources/nString/String_Loop.c
  Sources/nString/String_ReplaceAt.c
  Sources/nString/String_Search.c
  Sources/nString/String_ToLower.c
  Sources/nString/String_UTF8Encoder.c
  Sources/nString/String_Compare.c
  Sources/nString/String_Constructor_Wcs.c
  Sources/nString/String_Extract.c
  Sources/nString/String_get.c
  Sources/nString/String_IsAlpha.c
  Sources/nString/String_IsDecimal.c
  Sources/nString/String_IsOctal.c
  Sources/nString/String_LastOfIndex.c
  Sources/nString/String_Middle.c
  Sources/nString/String_Replace.c
  Sources/nString/String_set.c
  Sources/nString/String_ToUpper.c

  Sources/nString/STR/__ChsLen.c
  Sources/nString/STR/__IsAlpha.c
  Sources/nString/STR/__IsAlphaDigit.c
  Sources/nString/STR/__IsBinary.c
  Sources/nString/STR/__IsControl.c
  Sources/nString/STR/__IsDecimal.c
  Sources/nString/STR/__IsDigit.c
  Sources/nString/STR/__IsHex.c
  Sources/nString/STR/__IsLower.c
  Sources/nString/STR/__IsOctal.c
  Sources/nString/STR/__IsSpace.c
  Sources/nString/STR/__IsUpper.c
  Sources/nString/STR/__ToLower.c
  Sources/nString/STR/__ToUpper.c
  Sources/nString/STR/__WcsLen.c

  Sources/nString/ToString/String_ToString_Bool.c
  Sources/nString/ToString/String_ToString_Decimal.c
  Sources/nString/ToString/String_ToString_Decimal_Unsigned.c
  Sources/nString/ToString/String_ToString_Digit.c
  Sources/nString/ToString/String_ToString_StringAry.c

  Sources/nString/ValueOf/String_ValueOf_Bool.c
  Sources/nString/ValueOf/String_ValueOf_Decimal.c
  Sources/nString/ValueOf/String_ValueOf_Decimal_Unsigned.c
  Sources/nString/ValueOf/String_ValueOf_Digit.c

  Sources/nStringAry/StringAry_Constructor.c
  Sources/nStringAry/StringAry_Contains.c
  Sources/nStringAry/StringAry_Clear.c
  Sources/nStringAry/StringAry_CountIf.c
  Sources/nStringAry/StringAry_Destructor.c
  Sources/nStringAry/StringAry_get.c
  Sources/nStringAry/StringAry_set.c
  Sources/nStringAry/StringAry_Insert.c
  Sources/nStringAry/StringAry_Pop.c
  Sources/nStringAry/StringAry_Push.c
  Sources/nStringAry/StringAry_Remove.c
  Sources/nStringAry/StringAry_Search.c
  Sources/nStringAry/StringAry_toAry.c
  Sources/nStringAry/StringAry_toList.c
  Sources/nStringAry/StringAry_getType.c
  Sources/nStringAry/StringAry_NodeMake.c

)
set (nUtility_SRCS
  Sources/nPrint/nPrint.c
  Sources/nPrint/nPrintLine.c
  Sources/nPrint/nPrintErr.c
)


add_executable(nCompiler
  main.c

  ${nString_SRCS}
  ${nUtility_SRCS}
)

target_include_directories(
  nCompiler

PUBLIC
  Includes/
)

set ( CMAKE_C_FLAGS_DEBUG "-g -DDEBUG -Wall" )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
