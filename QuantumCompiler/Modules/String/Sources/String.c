
#include "String.h"
#include "Private_String.h"
#include "ProgramManager.h"

#include <locale.h>
#include <stdio.h>

struct StringMethod StringMethod;

// TODO 각종 전역 변수 및 모듈 초기화 함수를 통합

void StringModule_Initialized() {
  StringMethod.Join = String_Join;
  StringMethod.Append = String_Append;
  StringMethod.SubString = String_SubString;
  StringMethod.Loop = String_Loop;
  StringMethod.Split = String_Split;
  StringMethod.Compare = String_Compare;
  StringMethod.Trim = String_Trim;
  StringMethod.Contains = String_Contains;
  StringMethod.Count = String_Count;
  StringMethod.Get = String_Get;
  StringMethod.Set = String_Set;
  StringMethod.Length = String_Length;
  StringMethod.ToLower = String_ToLower;
  StringMethod.ToUpper = String_ToUpper;
  StringMethod.IsNone = String_IsNone;
  StringMethod.IndexOf = String_IndexOf;
  StringMethod.LastOfIndex = String_LastOfIndex;
  StringMethod.Replace = String_Replace;
  StringMethod.ReplaceFor = String_ReplaceFor;
  StringMethod.ReplaceAll = String_ReplaceAll;
  StringMethod.Left = String_Left;
  StringMethod.Right = String_Right;
  StringMethod.Middle = String_Middle;
  StringMethod.Const = String_Const;
  StringMethod.UnConst = String_UnConst;
  StringMethod.Destructor = String_Destructor;
  StringMethod.IndexFor = String_IndexFor;
  setlocale(LC_ALL, "");
}