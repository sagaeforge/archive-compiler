#ifndef __DATATYPES__
#define __DATATYPES__

#include <stdbool.h>

typedef unsigned int Length;
typedef unsigned int Index;

// [*]  [자료형 파트]
// [+ START] 자료형

#define OverloaddingTypes(Parameter)                                           \
  _Generic(Parameter,                                                          \
  char                  : "ch",                                                \
  unsigned char         : "un_ch",                                             \
  short                 : "sh",                                                \
  unsigned short        : "un_sh",                                             \
  int                   : "i",                                                 \
  unsigned int          : "un_i",                                              \
  long                  : "l",                                                 \
  unsigned long         : "un_l",                                              \
  long long             : "ll",                                                \
  unsigned long long    : "un_ll",                                             \
  char *                : "ptr_ch",                                            \
  char **               : "ptr2_ch",                                           \
  const char *          : "co_ptr_ch",                                         \
  const char **         : "co_ptr2_ch",                                        \
  int *                 : "ptr_i",                                             \
  int **                : "ptr2_i",                                            \
  const int *           : "co_ptr_i",                                          \
  const int **          : "co_ptr2_i",                                         \
  long *                : "ptr_l",                                             \
  long **               : "ptr2_l",                                            \
  const long *          : "co_ptr_l",                                          \
  const long **         : "co_ptr2_l",                                         \
  long long *           : "ptr_ll",                                            \
  long long **          : "ptr2_ll",                                           \
  const long long *     : "coptr_ll",                                          \
  const long long **    : "coptr2_ll",                                         \
  void *                : "ptr",                                               \
  void **               : "ptr2",                                              \
  void ***              : "ptr3",                                              \
  const void *          : "co_ptr",                                            \
  const void **         : "co_ptr2",                                           \
  _Bool                 : "b",                                                 \
  default: "unknown" )

#define Type(Instance)                                                         \
  _Generic(Instance,                                                           \
  char                  : "char",                                              \
  unsigned char         : "unsigned char",                                     \
  short                 : "short",                                             \
  unsigned short        : "unsigned short",                                    \
  int                   : "int",                                               \
  unsigned int          : "unsigned int",                                      \
  long                  : "long",                                              \
  unsigned long         : "unsigned long",                                     \
  long long             : "long long",                                         \
  unsigned long long    : "unsigned long long",                                \
  char *                : "char *",                                            \
  char **               : "char **",                                           \
  const char *          : "const char *",                                      \
  const char **         : "const char **",                                     \
  int *                 : "int *",                                             \
  int **                : "int **",                                            \
  const int *           : "const int *",                                       \
  const int **          : "const int **",                                      \
  long *                : "long *",                                            \
  long **               : "long **",                                           \
  const long *          : "const long *",                                      \
  const long **         : "const long **",                                     \
  long long *           : "long long *",                                       \
  long long **          : "long long **",                                      \
  const long long *     : "const long long *",                                 \
  const long long **    : "const long long **",                                \
  void *                : "void *",                                            \
  void **               : "void **",                                           \
  void ***              : "void ***",                                          \
  const void *          : "const void *",                                      \
  const void **         : "const void **",                                     \
  _Bool                 : "_Bool",                                             \
  default: "unknown" )

// [Public] TypeCompare(Instance1: 변수, Instance2: 변수),
// [Public] 두 변수의 자료형을 비교합니다.
#define TypeCompare(Instance1, Instance2)                                      \
  strcmp(GetType(Instance1), GetType(Instance2)) == 0

// [+ END] 자료형 끝

#define LengthCalc(DataType, Count) (sizeof(DataType) * Count)

#endif