cmake_minimum_required(VERSION 3.0.0)
project(JSON VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  JSON
  Sources/Json.c
  Sources/Json.c
  Sources/JSON_Clone.c
  Sources/JSON_Compare.c
  Sources/JSON_Export.c
  Sources/JSON_Find.c
  Sources/JSON_IsFieldOf.c
  Sources/JSON_IsTypeOf.c
  Sources/JSON_Print.c
  Sources/JSON_Remove.c
  Sources/JSON_RemoveAryAt.c
  Sources/JSON_Search.c
  Sources/JSON_Sum.c

  Sources/Append/JSON_AppendAry.c
  Sources/Append/JSON_AppendBool.c
  Sources/Append/JSON_AppendDeciaml.c
  Sources/Append/JSON_AppendDigit.c
  Sources/Append/JSON_AppendNULL.c
  Sources/Append/JSON_AppendObject.c
  Sources/Append/JSON_AppendString.c
  Sources/Append/JSON_AppendAryAry.c
  Sources/Append/JSON_AppendBoolAry.c
  Sources/Append/JSON_AppendDeciamlAry.c
  Sources/Append/JSON_AppendDigitAry.c
  Sources/Append/JSON_AppendObjectAry.c
  Sources/Append/JSON_AppendStringAry.c

  Sources/Get/JSON_GetAry.c
  Sources/Get/JSON_GetBool.c
  Sources/Get/JSON_GetDeciaml.c
  Sources/Get/JSON_GetDigit.c
  Sources/Get/JSON_GetFiledLength.c
  Sources/Get/JSON_GetNULL.c
  Sources/Get/JSON_GetObject.c
  Sources/Get/JSON_GetString.c
  Sources/Get/JSON_GetType.c

  Sources/Set/JSON_SetAry.c
  Sources/Set/JSON_SetBool.c
  Sources/Set/JSON_SetDeciaml.c
  Sources/Set/JSON_SetDigit.c
  Sources/Set/JSON_SetNULL.c
  Sources/Set/JSON_SetObject.c
  Sources/Set/JSON_SetString.c

  Sources/Read/JSON_Read_File.c
  Sources/Read/JSON_Read_StrAry.c
  Sources/Read/JSON_Read_Str.c

  Sources/Write/JSON_Write_File.c
  Sources/Write/JSON_Write_StrAry.c
  Sources/Write/JSON_Write_Str.c
)

target_include_directories(
  JSON
  PUBLIC

  ../Application/Includes/Public/
  ../String/Includes/Public/

  Includes/Public/
  Includes/Private/
)
add_compile_definitions(
  DEBUG
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
