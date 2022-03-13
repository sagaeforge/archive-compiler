cmake_minimum_required ( VERSION 3.0.0 )
project ( nCX_Lib VERSION 0.0.1 )

include ( CTest )
enable_testing ()

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/exec )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${PROJECT_NAME}/${CMAKE_BUILD_TYPE}/${PROJECT_VERSION}/lib )

set ( nGarbageCollection_SRCS
  nGarbageCollection.c
)

set ( nString_SRCS
  nString.c

  Sources/nChs/nChs_ChsLen.c
  Sources/nChs/nChs_IsAlpha.c
  Sources/nChs/nChs_IsAlphaDigit.c
  Sources/nChs/nChs_IsBinary.c
  Sources/nChs/nChs_IsControl.c
  Sources/nChs/nChs_IsDecimal.c
  Sources/nChs/nChs_IsDigit.c
  Sources/nChs/nChs_IsHex.c
  Sources/nChs/nChs_IsLower.c
  Sources/nChs/nChs_IsOctal.c
  Sources/nChs/nChs_IsSpace.c
  Sources/nChs/nChs_IsUpper.c
  Sources/nChs/nChs_ToLower.c
  Sources/nChs/nChs_ToUpper.c
  Sources/nChs/nChs_WcsLen.c

  Sources/nRegExp/RegExp_Analysis.c
  Sources/nRegExp/RegExp_Constructor_Clone.c
  Sources/nRegExp/RegExp_Constructor_Str.c
  Sources/nRegExp/RegExp_Count.c
  Sources/nRegExp/RegExp_Destructor.c
  Sources/nRegExp/RegExp_IndexAll.c
  Sources/nRegExp/RegExp_IndexAt.c
  Sources/nRegExp/RegExp_IndexFor.c
  Sources/nRegExp/RegExp_IndexOf.c
  Sources/nRegExp/RegExp_LastOfIndex.c
  Sources/nRegExp/RegExp_ReplaceAll.c
  Sources/nRegExp/RegExp_ReplaceAt.c
  Sources/nRegExp/RegExp_Replace.c
  Sources/nRegExp/RegExp_Split.c
  Sources/nRegExp/RegExp_SubString.c

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
  Sources/nString/String_get.c
  Sources/nString/String_IndexAt.c
  Sources/nString/String_IndexFor.c
  Sources/nString/String_IndexOf.c
  Sources/nString/String_isAlpha.c
  Sources/nString/String_isAlphaDigit.c
  Sources/nString/String_isBinary.c
  Sources/nString/String_isControl.c
  Sources/nString/String_isDecimal.c
  Sources/nString/String_isDigit.c
  Sources/nString/String_isHex.c
  Sources/nString/String_isLower.c
  Sources/nString/String_isOctal.c
  Sources/nString/String_isSpace.c
  Sources/nString/String_isUpper.c
  Sources/nString/String_Join.c
  Sources/nString/String_LastOfIndex.c
  Sources/nString/String_Left.c
  Sources/nString/String_Length.c
  Sources/nString/String_Loop.c
  Sources/nString/String_Middle.c
  Sources/nString/String_Notation.c
  Sources/nString/String_Pattern.c
  Sources/nString/String_ReplaceAll.c
  Sources/nString/String_ReplaceAt.c
  Sources/nString/String_Replace.c
  Sources/nString/String_Reverse.c
  Sources/nString/String_Right.c
  Sources/nString/String_Search.c
  Sources/nString/String_set.c
  Sources/nString/String_Split.c
  Sources/nString/String_SubString.c
  Sources/nString/String_ToLower.c
  Sources/nString/String_ToUpper.c
  Sources/nString/String_Trim.c
  Sources/nString/String_UTF8Decoder.c
  Sources/nString/String_UTF8Encoder.c

  Sources/nStringAry/StringAry_Clear.c
  Sources/nStringAry/StringAry_Constructor.c
  Sources/nStringAry/StringAry_Contains.c
  Sources/nStringAry/StringAry_CountIf.c
  Sources/nStringAry/StringAry_Destructor.c
  Sources/nStringAry/StringAry_get.c
  Sources/nStringAry/StringAry_Insert.c
  Sources/nStringAry/StringAry_Length.c
  Sources/nStringAry/StringAry_Pop.c
  Sources/nStringAry/StringAry_Push.c
  Sources/nStringAry/StringAry_Remove.c
  Sources/nStringAry/StringAry_Search.c
  Sources/nStringAry/StringAry_set.c
  Sources/nStringAry/StringAry_toAry.c
  Sources/nStringAry/StringAry_toList.c
  Sources/nStringAry/StringAry_Type.c


  Sources/String_toStringSearch.c
  Sources/String_ValueOfSearch.c
)

add_library ( nCX_Lib
  nCX_Library.c

  ${nGarbageCollection_SRCS}
  ${nString_SRCS}
)

target_include_directories ( nCX_Lib
PUBLIC
  Includes/nString/
  Includes/nGarbageCollection/
)

set ( CPACK_PROJECT_NAME ${PROJECT_NAME} )
set ( CPACK_PROJECT_VERSION ${PROJECT_VERSION} )
include ( CPack )
