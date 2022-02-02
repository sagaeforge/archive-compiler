
#ifndef __PUBLIC_APPLICATION_EXCEPTION__
#define __PUBLIC_APPLICATION_EXCEPTION__

#define WARNING 0
#define ERROR 1

#define WANNINGTCOLOR 93
#define ERRORCOLOR 91
#define MESSAGECOLOR 90

#define DEBUG

#ifdef DEBUG
#define Exception(errorcode, format, args...)                                  \
  do {                                                                         \
    if (errorcode == WARNING) {                                                \
      fprintf(stderr, "\x1b[%dm[WARNING]\x1b[0m \n", WANNINGTCOLOR);           \
      fprintf(stderr,                                                          \
              "\x1b[%dm=====================================\x1b[0m\n",        \
              WANNINGTCOLOR);                                                  \
    } else if (errorcode == ERROR) {                                           \
      fprintf(stderr, "\x1b[%dm[ERROR]\x1b[0m \n", ERRORCOLOR);                \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\x1b[0m\n",     \
        ERRORCOLOR);                                                           \
    } else {                                                                   \
      fprintf(stderr, "잘못된 ERRORCODE 입니다.");                             \
      break;                                                                   \
    }                                                                          \
    fprintf(stderr, " + [%-7s%6s] %s\n", "FILE", "NAME", __FILE__);            \
    fprintf(stderr, " + [%-13s] %s()\n", "FUNCTION NAME", __FUNCTION__);       \
    fprintf(stderr, " + [%-7s%6s] %d\n", "LINE", "NUMS", __LINE__);            \
    fprintf(stderr, "\n");                                                     \
    fprintf(stderr, "\x1b[%dm[MESSAGE]\x1b[0m: \n", MESSAGECOLOR);             \
    fprintf(stderr, format, ##args);                                           \
    fprintf(stderr, "\n");                                                     \
    if (errorcode == WARNING) {                                                \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\\x1b[0m\n",    \
        WANNINGTCOLOR);                                                        \
    } else if (errorcode == ERROR) {                                           \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\x1b[0m\n",     \
        ERRORCOLOR);                                                           \
    }                                                                          \
  } while (0)
#else
#define Exception(errorcode, format, args...)                                  \
  do {                                                                         \
    if (errorcode == WARNING) {                                                \
      fprintf(stderr, "\x1b[%dm[WARNING]\x1b[0m \n", WANNINGTCOLOR);           \
      fprintf(stderr,                                                          \
              "\x1b[%dm=====================================\x1b[0m\n",        \
              WANNINGTCOLOR);                                                  \
      fprintf(stderr, "\x1b[%dm[WARNING MESSAGE]\x1b[0m: \n", MESSAGECOLOR);   \
    } else if (errorcode == ERROR) {                                           \
      fprintf(stderr, "\x1b[%dm[ERROR]\x1b[0m \n", ERRORCOLOR);                \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\x1b[0m\n",     \
        ERRORCOLOR);                                                           \
      fprintf(stderr, "\x1b[%dm[ERROR MESSAGE]\x1b[0m: \n", MESSAGECOLOR);     \
    } else {                                                                   \
      fprintf(stderr, "잘못된 ERRORCODE 입니다.");                             \
      break;                                                                   \
    }                                                                          \
    fprintf(stderr, format, ##args);                                           \
    fprintf(stderr, "\n");                                                     \
    if (errorcode == WARNING) {                                                \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\\x1b[0m\n",    \
        WANNINGTCOLOR);                                                        \
    } else if (errorcode == ERROR) {                                           \
      fprintf(                                                                 \
        stderr,                                                                \
        "\x1b[%dm==============================================\x1b[0m\n",     \
        ERRORCOLOR);                                                           \
    }                                                                          \
  } while (0)

#endif
#endif