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

  Sources/nStringModule/nChs/nChs_ChsLen.c
  Sources/nStringModule/nChs/nChs_IsAlpha.c
  Sources/nStringModule/nChs/nChs_IsAlphaDigit.c
  Sources/nStringModule/nChs/nChs_IsBinary.c
  Sources/nStringModule/nChs/nChs_IsControl.c
  Sources/nStringModule/nChs/nChs_IsDecimal.c
  Sources/nStringModule/nChs/nChs_IsDigit.c
  Sources/nStringModule/nChs/nChs_IsHex.c
  Sources/nStringModule/nChs/nChs_IsLower.c
  Sources/nStringModule/nChs/nChs_IsOctal.c
  Sources/nStringModule/nChs/nChs_IsSpace.c
  Sources/nStringModule/nChs/nChs_IsUpper.c
  Sources/nStringModule/nChs/nChs_ToLower.c
  Sources/nStringModule/nChs/nChs_ToUpper.c
  Sources/nStringModule/nChs/nChs_WcsLen.c

  Sources/nStringModule/nRegExp/RegExp_Analysis.c
  Sources/nStringModule/nRegExp/RegExp_Constructor_Clone.c
  Sources/nStringModule/nRegExp/RegExp_Constructor_Str.c
  Sources/nStringModule/nRegExp/RegExp_Count.c
  Sources/nStringModule/nRegExp/RegExp_Destructor.c
  Sources/nStringModule/nRegExp/RegExp_IndexAll.c
  Sources/nStringModule/nRegExp/RegExp_IndexAt.c
  Sources/nStringModule/nRegExp/RegExp_IndexFor.c
  Sources/nStringModule/nRegExp/RegExp_IndexOf.c
  Sources/nStringModule/nRegExp/RegExp_LastOfIndex.c
  Sources/nStringModule/nRegExp/RegExp_ReplaceAll.c
  Sources/nStringModule/nRegExp/RegExp_ReplaceAt.c
  Sources/nStringModule/nRegExp/RegExp_Replace.c
  Sources/nStringModule/nRegExp/RegExp_Split.c
  Sources/nStringModule/nRegExp/RegExp_SubString.c

  Sources/nStringModule/nString/String_Append.c
  Sources/nStringModule/nString/String_Check.c
  Sources/nStringModule/nString/String_Compare.c
  Sources/nStringModule/nString/String_Constructor_Chs.c
  Sources/nStringModule/nString/String_Constructor_Str.c
  Sources/nStringModule/nString/String_Constructor_Strp.c
  Sources/nStringModule/nString/String_Constructor_Wcs.c
  Sources/nStringModule/nString/String_Contains.c
  Sources/nStringModule/nString/String_Count.c
  Sources/nStringModule/nString/String_Destructor.c
  Sources/nStringModule/nString/String_Extract.c
  Sources/nStringModule/nString/String_FileAllRead.c
  Sources/nStringModule/nString/String_FileAllWrite.c
  Sources/nStringModule/nString/String_Format.c
  Sources/nStringModule/nString/String_get.c
  Sources/nStringModule/nString/String_IndexAt.c
  Sources/nStringModule/nString/String_IndexFor.c
  Sources/nStringModule/nString/String_IndexOf.c
  Sources/nStringModule/nString/String_isAlpha.c
  Sources/nStringModule/nString/String_isAlphaDigit.c
  Sources/nStringModule/nString/String_isBinary.c
  Sources/nStringModule/nString/String_isControl.c
  Sources/nStringModule/nString/String_isDecimal.c
  Sources/nStringModule/nString/String_isDigit.c
  Sources/nStringModule/nString/String_isHex.c
  Sources/nStringModule/nString/String_isLower.c
  Sources/nStringModule/nString/String_isOctal.c
  Sources/nStringModule/nString/String_isSpace.c
  Sources/nStringModule/nString/String_isUpper.c
  Sources/nStringModule/nString/String_Join.c
  Sources/nStringModule/nString/String_LastOfIndex.c
  Sources/nStringModule/nString/String_Left.c
  Sources/nStringModule/nString/String_Length.c
  Sources/nStringModule/nString/String_Loop.c
  Sources/nStringModule/nString/String_Middle.c
  Sources/nStringModule/nString/String_Notation.c
  Sources/nStringModule/nString/String_Pattern.c
  Sources/nStringModule/nString/String_ReplaceAll.c
  Sources/nStringModule/nString/String_ReplaceAt.c
  Sources/nStringModule/nString/String_Replace.c
  Sources/nStringModule/nString/String_Reverse.c
  Sources/nStringModule/nString/String_Right.c
  Sources/nStringModule/nString/String_set.c
  Sources/nStringModule/nString/String_Split.c
  Sources/nStringModule/nString/String_SubString.c
  Sources/nStringModule/nString/String_ToLower.c
  Sources/nStringModule/nString/String_ToUpper.c
  Sources/nStringModule/nString/String_Trim.c
  Sources/nStringModule/nString/String_UTF8Decoder.c
  Sources/nStringModule/nString/String_UTF8Encoder.c

  Sources/nStringModule/nStringAry/StringAry_Clear.c
  Sources/nStringModule/nStringAry/StringAry_Constructor.c
  Sources/nStringModule/nStringAry/StringAry_Contains.c
  Sources/nStringModule/nStringAry/StringAry_CountIf.c
  Sources/nStringModule/nStringAry/StringAry_Destructor.c
  Sources/nStringModule/nStringAry/StringAry_get.c
  Sources/nStringModule/nStringAry/StringAry_Insert.c
  Sources/nStringModule/nStringAry/StringAry_Length.c
  Sources/nStringModule/nStringAry/StringAry_Pop.c
  Sources/nStringModule/nStringAry/StringAry_Push.c
  Sources/nStringModule/nStringAry/StringAry_Remove.c
  Sources/nStringModule/nStringAry/StringAry_Search.c
  Sources/nStringModule/nStringAry/StringAry_set.c
  Sources/nStringModule/nStringAry/StringAry_toAry.c
  Sources/nStringModule/nStringAry/StringAry_toList.c
  Sources/nStringModule/nStringAry/StringAry_Type.c

  Sources/nStringModule/String_toStringSearch.c
  Sources/nStringModule/String_ValueOfSearch.c
)

add_library ( nCX_Lib
  nCX_Library.c

  ${nGarbageCollection_SRCS}
  ${nString_SRCS}
)

target_include_directories ( nCX_Lib
PUBLIC
  Includes/
)

set ( CPACK_PROJECT_NAME ${PROJECT_NAME} )
set ( CPACK_PROJECT_VERSION ${PROJECT_VERSION} )
include ( CPack )
