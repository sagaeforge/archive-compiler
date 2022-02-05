cmake_minimum_required(VERSION 3.0.0)
project(JSON VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  JSON
  Sources/Json.c
  Sources/JSON/JSON_Constructor.c
  Sources/JSON/JSON_Destructor.c
  Sources/JSON/JSON_NodeCreate.c
  Sources/JSON/JSON_Append.c
  Sources/JSON/JSON_Clone.c
  Sources/JSON/JSON_Constructor.c
  Sources/JSON/JSON_Export.c
  Sources/JSON/JSON_FiledLength.c
  Sources/JSON/JSON_Parent.c
  Sources/JSON/JSON_Remove.c
  Sources/JSON/JSON_SetParent.c
  Sources/JSON/JSON_TypeOf.c
  Sources/JSON/JSON_Clear.c
  Sources/JSON/JSON_Compare.c
  Sources/JSON/JSON_Destructor.c
  Sources/JSON/JSON_FieldOf.c
  Sources/JSON/JSON_Get.c
  Sources/JSON/JSON_Object.c
  Sources/JSON/JSON_Print.c
  Sources/JSON/JSON_Type.c
  Sources/JSON/JSON_Set.c

  Sources/JSONAry/JSONAry_Constructor.c
  Sources/JSONAry/JSONAry_Destructor.c
  Sources/JSONAry/JSONAry_Clear.c
  Sources/JSONAry/JSONAry_Compare.c
  Sources/JSONAry/JSONAry_Constructor.c
  Sources/JSONAry/JSONAry_Contains.c
  Sources/JSONAry/JSONAry_Destructor.c
  Sources/JSONAry/JSONAry_Get.c
  Sources/JSONAry/JSONAry_Insert.c
  Sources/JSONAry/JSONAry_Length.c
  Sources/JSONAry/JSONAry_NodeCreate.c
  Sources/JSONAry/JSONAry_Object.c
  Sources/JSONAry/JSONAry_Pop.c
  Sources/JSONAry/JSONAry_Push.c
  Sources/JSONAry/JSONAry_Remove.c
  Sources/JSONAry/JSONAry_Set.c
  Sources/JSONAry/JSONAry_SetObject.c

  Sources/JSON/Read/JSON_Read_File.c
  Sources/JSON/Read/JSON_Read_StrAry.c
  Sources/JSON/Read/JSON_Read_Str.c
  Sources/JSON/Write/JSON_Write_File.c
  Sources/JSON/Write/JSON_Write_StrAry.c
  Sources/JSON/Write/JSON_Write_Str.c

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
target_compile_options(
  JSON
  PUBLIC
  
  # -v
)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
