#ifndef __EXCEPTION__
#define __EXCEPTION__

#include <stdio.h>
#include <string.h>

#ifdef DEBUG
#define Debug(format, args...)                                                 \
  do {                                                                         \
    fprintf(stderr, "\n ====== DEBUG ====== \n");                              \
    fprintf(stderr, "[%s:%d]:%s()\n", __FILE__, __LINE__, __FUNCTION__);       \
    fprintf(stderr, format, ##args);                                           \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#define Warning(format, args...)                                               \
  do {                                                                         \
    Debug("오류 위치 검사: ");                                                 \
    fprintf(stderr, "Warning >>> ");                                           \
    fprintf(stderr, format, ##args);                                           \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#else
#define Warning(format, args...)                                               \
  do {                                                                         \
    printf("Warning >>> ");                                                    \
    fprintf(stderr, format, ##args);                                           \
    printf("\n");                                                              \
  } while (0)
#endif

#define Error(format, args...)                                                 \
  do {                                                                         \
    printf("Error >>> ");                                                      \
    fprintf(stderr, format, ##args);                                           \
    printf("\n");                                                              \
    exit(1);                                                                   \
  } while (0)

#endif