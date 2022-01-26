
#ifndef __PRIVATE_APPLICATION_DATATYPE__
#define __PRIVATE_APPLICATION_DATATYPE__

#include <stdbool.h>
#include <stdint.h>

#define SystemType(Instance)                                                   \
  _Generic((Instance),                                                         \
  char                        : "char",                                        \
  unsigned char               : "unsigned char",                               \
  short                       : "short",                                       \
  unsigned short              : "unsigned short",                              \
  int                         : "int",                                         \
  unsigned int                : "unsigned int",                                \
  long                        : "long",                                        \
  unsigned long               : "unsigned long",                               \
  long long                   : "long long",                                   \
  unsigned long long          : "unsigned long long",                          \
  float                       : "float",                                       \
  double                      : "double",                                      \
  char*                       : "char*",                                       \
  char**                      : "char**",                                      \
  char***                     : "char***",                                     \
  const char*                 : "const char*",                                 \
  const char**                : "const char**",                                \
  const char***               : "const char***",                               \
  unsigned char*              : "unsigned char*",                              \
  unsigned char**             : "unsigned char**",                             \
  unsigned char***            : "unsigned char***",                            \
  const unsigned char*        : "const unsigned char*",                        \
  const unsigned char**       : "const unsigned char**",                       \
  const unsigned char***      : "const unsigned char***",                      \
  short*                      : "short*",                                      \
  short**                     : "short**",                                     \
  short***                    : "short***",                                    \
  const short*                : "const short*",                                \
  const short**               : "const short**",                               \
  const short***              : "const short***",                              \
  unsigned short*             : "unsigned short*",                             \
  unsigned short**            : "unsigned short**",                            \
  unsigned short***           : "unsigned short***",                           \
  const unsigned short*       : "const unsigned short*",                       \
  const unsigned short**      : "const unsigned short**",                      \
  const unsigned short***     : "const unsigned short***",                     \
  int*                        : "int*",                                        \
  int**                       : "int**",                                       \
  int***                      : "int***",                                      \
  const int*                  : "const int*",                                  \
  const int**                 : "const int**",                                 \
  const int***                : "const int***",                                \
  unsigned int*               : "unsigned int*",                               \
  unsigned int**              : "unsigned int**",                              \
  unsigned int***             : "unsigned int***",                             \
  const unsigned int*         : "const unsigned int*",                         \
  const unsigned int**        : "const unsigned int**",                        \
  const unsigned int***       : "const unsigned int***",                       \
  long*                       : "long*",                                       \
  long**                      : "long**",                                      \
  long***                     : "long***",                                     \
  const long*                 : "const long*",                                 \
  const long**                : "const long**",                                \
  const long***               : "const long***",                               \
  unsigned long*              : "unsigned long*",                              \
  unsigned long**             : "unsigned long**",                             \
  unsigned long***            : "unsigned long***",                            \
  const unsigned long*        : "const unsigned long*",                        \
  const unsigned long**       : "const unsigned long**",                       \
  const unsigned long***      : "const unsigned long***",                      \
  long long*                  : "long long*",                                  \
  long long**                 : "long long**",                                 \
  long long***                : "long long***",                                \
  const long long*            : "const long long*",                            \
  const long long**           : "const long long**",                           \
  const long long***          : "const long long***",                          \
  unsigned long long*         : "unsigned long long*",                         \
  unsigned long long**        : "unsigned long long**",                        \
  unsigned long long***       : "unsigned long long***",                       \
  const unsigned long long*   : "const unsigned long long*",                   \
  const unsigned long long**  : "const unsigned long long**",                  \
  const unsigned long long*** : "const unsigned long long***",                 \
  float*                      : "float*",                                      \
  float**                     : "float**",                                     \
  float***                    : "float***",                                    \
  const float*                : "const float*",                                \
  const float**               : "const float**",                               \
  const float***              : "const float***",                              \
  double*                     : "double*",                                     \
  double**                    : "double**",                                    \
  double***                   : "double***",                                   \
  const double*               : "const double*",                               \
  const double**              : "const double**",                              \
  const double***             : "const double***",                             \
  void*                       : "void*",                                       \
  void**                      : "void**",                                      \
  void***                     : "void***",                                     \
  const void*                 : "const void*",                                 \
  const void**                : "const void**",                                \
  const void***               : "const void***",                               \
  default                     : "Unknown")

#endif