
#include <Application.h>
#include <Private_String.h>
#include <Private_StringAry.h>
#include <Private_StringLib.h>
#include <String.h>

#include <locale.h>
#include <stdio.h>

struct StringMethod StringMethod;
struct StringLibMethod StringLibMethod;
struct StringAryMethod StringAryMethod;

void
StringModule_Initialized()
{
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
  StringMethod.Destructor = String_Destructor;
  StringMethod.IndexFor = String_IndexFor;
  StringMethod.IndexAt = String_IndexAt;

  StringLibMethod.Extract = String_Extract;
  StringLibMethod.Format = String_Format;
  StringLibMethod.IsAlpha = String_IsAlpha;
  StringLibMethod.IsAlphaDigit = String_IsAlphaDigit;
  StringLibMethod.IsBinary = String_IsBinary;
  StringLibMethod.IsControl = String_IsControl;
  StringLibMethod.IsDecimal = String_IsDecimal;
  StringLibMethod.IsDigit = String_IsDigit;
  StringLibMethod.IsHex = String_IsHex;
  StringLibMethod.IsLower = String_IsLower;
  StringLibMethod.IsOctal = String_IsOctal;
  StringLibMethod.IsSpace = String_IsSpace;
  StringLibMethod.IsUpper = String_IsUpper;
  StringLibMethod.Notation = String_Notation;
  StringLibMethod.Pattern = String_Pattern;
  StringLibMethod.Reverse = String_Reverse;
  StringLibMethod.FileAllRead = String_FileAllRead;
  StringLibMethod.FileAllWrite = String_FileAllWrite;

  StringAryMethod.Destructor = StringAryDestructor;
  StringAryMethod.Contains = StringAry_Contains;
  StringAryMethod.Get = StringAry_Get;
  StringAryMethod.Insert = StringAry_Insert;
  StringAryMethod.Pop = StringAry_Pop;
  StringAryMethod.Push = StringAry_Push;
  StringAryMethod.Remove = StringAry_Remove;
  StringAryMethod.Search = StringAry_Search;

  setlocale(LC_ALL, "");
}