cmake_minimum_required(VERSION 3.0.0)
project(String VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  String

  Sources/Chs.c
  Sources/String.c
  Sources/StringConstructor.c
  Sources/StringMethod.c
  Sources/StringInitialized.c
  Sources/StringLib.c

  Sources/StringMethod/String_Append.c   
  Sources/StringMethod/String_Contains.c    
  Sources/StringMethod/String_Get.c      
  Sources/StringMethod/String_Join.c         
  Sources/StringMethod/String_Length.c  
  Sources/StringMethod/String_Replace.c     
  Sources/StringMethod/String_Right.c  
  Sources/StringMethod/String_SubString.c  
  Sources/StringMethod/String_Trim.c
  Sources/StringMethod/String_Compare.c  
  Sources/StringMethod/String_Count.c       
  Sources/StringMethod/String_IndexOf.c  
  Sources/StringMethod/String_LastOfIndex.c  
  Sources/StringMethod/String_Loop.c    
  Sources/StringMethod/String_ReplaceAll.c  
  Sources/StringMethod/String_Set.c    
  Sources/StringMethod/String_ToLower.c    
  Sources/StringMethod/String_UnConst.c
  Sources/StringMethod/String_Const.c    
  Sources/StringMethod/String_Destructor.c  
  Sources/StringMethod/String_IsNone.c   
  Sources/StringMethod/String_Left.c         
  Sources/StringMethod/String_Middle.c  
  Sources/StringMethod/String_ReplaceFor.c  
  Sources/StringMethod/String_Split.c  
  Sources/StringMethod/String_ToUpper.c
)

target_include_directories(
  String
  PUBLIC

  # ProgramManager
  ../ProgramManager/Includes/
  ../ProgramManager/Module/ProcessEvent/Includes/Public/
  ../ProgramManager/Module/GarbageCollection/Includes/Public/
  ../ProgramManager/Module/InputSystem/Includes/Public/
  ../ProgramManager/Module/Exception/Includes/Public/

  # String
  Includes/Public/
  Includes/Private/
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
