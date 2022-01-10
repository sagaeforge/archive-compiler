#ifndef __DATATYPES__
#define __DATATYPES__

#include <stdbool.h>

typedef unsigned int Length;
typedef unsigned int Index;
typedef long long int _int64;
typedef unsigned long long int _uint64;

// [*]  [자료형 파트]
// [+ START] 자료형

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

#define TypeCompare(Instance1, Instance2)                                      \
  strcmp(GetType(Instance1), GetType(Instance2)) == 0

// [+ END] 자료형 끝

#define LengthCalc(DataType, Count) (sizeof(DataType) * Count)

#endif