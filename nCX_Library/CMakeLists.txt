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

  Sources/nStringModule/nRegExp/nRegExp_Analysis.c
  Sources/nStringModule/nRegExp/nRegExp_Constructor_Clone.c
  Sources/nStringModule/nRegExp/nRegExp_Constructor_Str.c
  Sources/nStringModule/nRegExp/nRegExp_Count.c
  Sources/nStringModule/nRegExp/nRegExp_Destructor.c
  Sources/nStringModule/nRegExp/nRegExp_IndexAll.c
  Sources/nStringModule/nRegExp/nRegExp_IndexAt.c
  Sources/nStringModule/nRegExp/nRegExp_IndexFor.c
  Sources/nStringModule/nRegExp/nRegExp_IndexOf.c
  Sources/nStringModule/nRegExp/nRegExp_LastOfIndex.c
  Sources/nStringModule/nRegExp/nRegExp_ReplaceAll.c
  Sources/nStringModule/nRegExp/nRegExp_ReplaceAt.c
  Sources/nStringModule/nRegExp/nRegExp_Replace.c
  Sources/nStringModule/nRegExp/nRegExp_Split.c
  Sources/nStringModule/nRegExp/nRegExp_SubString.c

  Sources/nStringModule/nString/nString_Append.c
  Sources/nStringModule/nString/nString_Check.c
  Sources/nStringModule/nString/nString_Compare.c
  Sources/nStringModule/nString/nString_Constructor_Chs.c
  Sources/nStringModule/nString/nString_Constructor_Str.c
  Sources/nStringModule/nString/nString_Constructor_Strp.c
  Sources/nStringModule/nString/nString_Constructor_Wcs.c
  Sources/nStringModule/nString/nString_Contains.c
  Sources/nStringModule/nString/nString_Count.c
  Sources/nStringModule/nString/nString_Destructor.c
  Sources/nStringModule/nString/nString_Extract.c
  Sources/nStringModule/nString/nString_FileAllRead.c
  Sources/nStringModule/nString/nString_FileAllWrite.c
  Sources/nStringModule/nString/nString_Format.c
  Sources/nStringModule/nString/nString_get.c
  Sources/nStringModule/nString/nString_IndexAt.c
  Sources/nStringModule/nString/nString_IndexFor.c
  Sources/nStringModule/nString/nString_IndexOf.c
  Sources/nStringModule/nString/nString_isAlpha.c
  Sources/nStringModule/nString/nString_isAlphaDigit.c
  Sources/nStringModule/nString/nString_isBinary.c
  Sources/nStringModule/nString/nString_isControl.c
  Sources/nStringModule/nString/nString_isDecimal.c
  Sources/nStringModule/nString/nString_isDigit.c
  Sources/nStringModule/nString/nString_isHex.c
  Sources/nStringModule/nString/nString_isLower.c
  Sources/nStringModule/nString/nString_isOctal.c
  Sources/nStringModule/nString/nString_isSpace.c
  Sources/nStringModule/nString/nString_isUpper.c
  Sources/nStringModule/nString/nString_Join.c
  Sources/nStringModule/nString/nString_LastOfIndex.c
  Sources/nStringModule/nString/nString_Left.c
  Sources/nStringModule/nString/nString_Length.c
  Sources/nStringModule/nString/nString_Loop.c
  Sources/nStringModule/nString/nString_Middle.c
  Sources/nStringModule/nString/nString_Notation.c
  Sources/nStringModule/nString/nString_Pattern.c
  Sources/nStringModule/nString/nString_ReplaceAll.c
  Sources/nStringModule/nString/nString_ReplaceAt.c
  Sources/nStringModule/nString/nString_Replace.c
  Sources/nStringModule/nString/nString_Reverse.c
  Sources/nStringModule/nString/nString_Right.c
  Sources/nStringModule/nString/nString_set.c
  Sources/nStringModule/nString/nString_Split.c
  Sources/nStringModule/nString/nString_SubString.c
  Sources/nStringModule/nString/nString_ToLower.c
  Sources/nStringModule/nString/nString_ToUpper.c
  Sources/nStringModule/nString/nString_Trim.c
  Sources/nStringModule/nString/nString_UTF8Decoder.c
  Sources/nStringModule/nString/nString_UTF8Encoder.c

  Sources/nStringModule/nStringAry/nStringAry_Clear.c
  Sources/nStringModule/nStringAry/nStringAry_Constructor.c
  Sources/nStringModule/nStringAry/nStringAry_Contains.c
  Sources/nStringModule/nStringAry/nStringAry_CountIf.c
  Sources/nStringModule/nStringAry/nStringAry_Destructor.c
  Sources/nStringModule/nStringAry/nStringAry_get.c
  Sources/nStringModule/nStringAry/nStringAry_Insert.c
  Sources/nStringModule/nStringAry/nStringAry_Length.c
  Sources/nStringModule/nStringAry/nStringAry_Pop.c
  Sources/nStringModule/nStringAry/nStringAry_Push.c
  Sources/nStringModule/nStringAry/nStringAry_Remove.c
  Sources/nStringModule/nStringAry/nStringAry_Search.c
  Sources/nStringModule/nStringAry/nStringAry_set.c
  Sources/nStringModule/nStringAry/nStringAry_toAry.c
  Sources/nStringModule/nStringAry/nStringAry_toList.c
  Sources/nStringModule/nStringAry/nStringAry_Type.c

  Sources/nStringModule/nString_toStringSearch.c
  Sources/nStringModule/nString_ValueOfSearch.c
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
