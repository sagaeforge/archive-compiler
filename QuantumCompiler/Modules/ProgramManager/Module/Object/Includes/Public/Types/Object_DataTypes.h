
#ifndef __PUBLIC_DATATYPES_OBJECT_DATATYPES__
#define __PUBLIC_DATATYPES_OBJECT_DATATYPES__

#include "DataTypes.h"
#include "Types/DataTypes_Object.h"

// [*] 사용자가 제공하는 ToObject 함수가 등록된 헤더를 등록하세요.
// [+ Start] 사용자 정의 헤더 함수 등록

// [+ End] 사용자 정의 헤더 함수 등록 끝

// clang-format off
#define ObjectMacroStart _Generic((Instance),
#define ObjectMacroEnd ) (Instance)

// 개별 객체에 대해서 생성자
#define Object(Instance)                  \
ObjectMacroStart                          \
char                : Virtual_Object    , \ 
short               : Virtual_Object    , \ 
int                 : Virtual_Object    , \ 
long                : Virtual_Object    , \ 
long long           : Virtual_Object    , \ 
unsigned char       : Virtual_Object    , \ 
unsigned short      : Virtual_Object    , \ 
unsigned int        : Virtual_Object    , \ 
unsigned long       : Virtual_Object    , \ 
unsigned long long  : Virtual_Object    , \ 
_Bool               : Virtual_Object    , \
default             : Virtual_Object      \
ObjectMacroEnd

#define Objects(Instance, Length)         \
ObjectMacroStart                          \
char*               : Virtual_Object    , \ 
short*              : Virtual_Object    , \ 
int*                : Virtual_Object    , \ 
long*               : Virtual_Object    , \ 
long long*          : Virtual_Object    , \ 
unsigned char*      : Virtual_Object    , \ 
unsigned short*     : Virtual_Object    , \ 
unsigned int*       : Virtual_Object    , \ 
unsigned long*      : Virtual_Object    , \ 
unsigned long long* : Virtual_Object    , \ 
default             : Virtual_Object      \
) (Instance, Length)
// clang-format on

// TODO 가상 함수 변경 하세요.
Object
Virtual_Object();

#endif
