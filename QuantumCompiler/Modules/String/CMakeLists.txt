cmake_minimum_required(VERSION 3.0.0)
project(String VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(
  String

  Sources/String.c
  Sources/StringConstructor.c
  Sources/StringAry.c

  Sources/Chs/Chslen.c
  Sources/Chs/WcsChsSet.c
  Sources/Chs/WcsCreate.c
  Sources/Chs/WcsWcsSet.c
  Sources/Chs/Wcslen.c
  Sources/Chs/ToUpper.c
  Sources/Chs/ToLower.c
  
  Sources/Chs/Is/IsAlpha.c
  Sources/Chs/Is/IsBinary.c
  Sources/Chs/Is/IsControl.c
  Sources/Chs/Is/IsDecimal.c
  Sources/Chs/Is/IsHex.c
  Sources/Chs/Is/IsLower.c
  Sources/Chs/Is/IsOctal.c
  Sources/Chs/Is/IsSpace.c
  Sources/Chs/Is/IsUpper.c


  Sources/StringLibrary/Is/String_IsAlpha.c
  Sources/StringLibrary/Is/String_IsAlphaDigit.c
  Sources/StringLibrary/Is/String_IsBinary.c
  Sources/StringLibrary/Is/String_IsControl.c
  Sources/StringLibrary/Is/String_IsDecimal.c
  Sources/StringLibrary/Is/String_IsDigit.c
  Sources/StringLibrary/Is/String_IsHex.c
  Sources/StringLibrary/Is/String_IsLower.c
  Sources/StringLibrary/Is/String_IsOctal.c
  Sources/StringLibrary/Is/String_IsSpace.c
  Sources/StringLibrary/Is/String_IsUpper.c

  Sources/StringLibrary/ToString/String_ToString_Bool.c
  Sources/StringLibrary/ToString/String_ToString_Decimal.c
  Sources/StringLibrary/ToString/String_ToString_Decimal_Unsigned.c
  Sources/StringLibrary/ToString/String_ToString_Digit.c

  Sources/StringLibrary/ValueOf/String_ValueOf_Bool.c
  Sources/StringLibrary/ValueOf/String_ValueOf_Decimal.c
  Sources/StringLibrary/ValueOf/String_ValueOf_Decimal_Unsigned.c
  Sources/StringLibrary/ValueOf/String_ValueOf_Digit.c

  Sources/StringLibrary/String_Extract.c
  Sources/StringLibrary/String_Format.c
  Sources/StringLibrary/String_Notation.c
  Sources/StringLibrary/String_Pattern.c
  Sources/StringLibrary/String_Reverse.c

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
  Sources/StringMethod/String_IndexFor.c  
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
