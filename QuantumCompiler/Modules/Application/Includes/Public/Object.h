
#ifndef __PUBLIC_APPLICATION_OBJECT__
#define __PUBLIC_APPLICATION_OBJECT__

#include <Types/DataType_Object.h>

#define __SystemObject(Instance)                                               \
  _Generic((Instance),                                                         \
  char                        : __Object_Boxing_Char,                          \
  unsigned char               : __Object_Boxing_U_Char,                        \
  short                       : __Object_Boxing_Short,                         \
  unsigned short              : __Object_Boxing_U_Short,                       \
  int                         : __Object_Boxing_Int,                           \
  unsigned int                : __Object_Boxing_U_Int,                         \
  long                        : __Object_Boxing_Long,                          \
  unsigned long               : __Object_Boxing_U_Long,                        \
  long long                   : __Object_Boxing_LongLong,                      \
  unsigned long long          : __Object_Boxing_U_LongLong,                    \
  float                       : __Object_Boxing_Float,                         \
  double                      : __Object_Boxing_Double,                        \
  char*                       : __Object_Boxing_Ptr_Char,                      \
  char**                      : __Object_Boxing_Double_Ptr_Char,               \
  const char*                 : __Object_Boxing_Ptr_Char,                      \
  const char**                : __Object_Boxing_Double_Ptr_Char,               \
  unsigned char*              : __Object_Boxing_Ptr_U_Char,                    \
  unsigned char**             : __Object_Boxing_Double_Ptr_U_Char,             \
  const unsigned char*        : __Object_Boxing_Ptr_U_Char,                    \
  const unsigned char**       : __Object_Boxing_Double_Ptr_U_Char,             \
  short*                      : __Object_Boxing_Ptr_Short,                     \
  short**                     : __Object_Boxing_Double_Ptr_Short,              \
  const short*                : __Object_Boxing_Ptr_Short,                     \
  const short**               : __Object_Boxing_Double_Ptr_Short,              \
  unsigned short*             : __Object_Boxing_Ptr_U_Short,                   \
  unsigned short**            : __Object_Boxing_Double_Ptr_U_Short,            \
  const unsigned short*       : __Object_Boxing_Ptr_U_Short,                   \
  const unsigned short**      : __Object_Boxing_Double_Ptr_U_Short,            \
  int*                        : __Object_Boxing_Ptr_Int,                       \
  int**                       : __Object_Boxing_Double_Ptr_Int,                \
  const int*                  : __Object_Boxing_Ptr_Int,                       \
  const int**                 : __Object_Boxing_Double_Ptr_Int,                \
  unsigned int*               : __Object_Boxing_Ptr_U_Int,                     \
  unsigned int**              : __Object_Boxing_Double_Ptr_U_Int,              \
  const unsigned int*         : __Object_Boxing_Ptr_U_Int,                     \
  const unsigned int**        : __Object_Boxing_Double_Ptr_U_Int,              \
  long*                       : __Object_Boxing_Ptr_Long,                      \
  long**                      : __Object_Boxing_Double_Ptr_Long,               \
  const long*                 : __Object_Boxing_Ptr_Long,                      \
  const long**                : __Object_Boxing_Double_Ptr_Long,               \
  unsigned long*              : __Object_Boxing_Ptr_U_Long,                    \
  unsigned long**             : __Object_Boxing_Double_Ptr_U_Long,             \
  const unsigned long*        : __Object_Boxing_Ptr_U_Long,                    \
  const unsigned long**       : __Object_Boxing_Double_Ptr_U_Long,             \
  long long*                  : __Object_Boxing_Ptr_Long_Long,                 \
  long long**                 : __Object_Boxing_Double_Ptr_Long_Long,          \
  const long long*            : __Object_Boxing_Ptr_Long_Long,                 \
  const long long**           : __Object_Boxing_Double_Ptr_Long_Long,          \
  unsigned long long*         : __Object_Boxing_Ptr_U_Long_Long,               \
  unsigned long long**        : __Object_Boxing_Double_Ptr_U_Long_Long,        \
  const unsigned long long*   : __Object_Boxing_Ptr_U_Long_Long,               \
  const unsigned long long**  : __Object_Boxing_Double_Ptr_U_Long_Long,        \
  float*                      : __Object_Boxing_Ptr_Float,                     \
  float**                     : __Object_Boxing_Double_Ptr_Float,              \
  const float*                : __Object_Boxing_Ptr_Float,                     \
  const float**               : __Object_Boxing_Double_Ptr_Float,              \
  double*                     : __Object_Boxing_Ptr_Double,                    \
  double**                    : __Object_Boxing_Double_Ptr_Double,             \
  const double*               : __Object_Boxing_Ptr_Double,                    \
  const double**              : __Object_Boxing_Double_Ptr_Double,             \
  void*                       : __Object_Boxing_Ptr_Void,                      \
  void**                      : __Object_Boxing_Double_Ptr_Void,               \
  void***                     : __Object_Boxing_Triple_Ptr_Void,               \
  const void*                 : __Object_Boxing_Ptr_Void,                      \
  const void**                : __Object_Boxing_Double_Ptr_Void,               \
  const void***               : __Object_Boxing_Triple_Ptr_Void                \
  _Bool                       : __Object_Boxing_Bool,                          \
  _Bool*                      : __Object_Boxing_Ptr_Bool,                      \
  _Bool**                     : __Object_Boxing_Double_Ptr_Bool,               \
  const _Bool*                : __Object_Boxing_Ptr_Bool,                      \
  const _Bool**               : __Object_Boxing_Double_Ptr_Bool,               \
  )

// clang-format off
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Char                      (const char                 pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Char                    (const unsigned char        pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Short                     (const short                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Short                   (const unsigned short       pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Int                       (const int                  pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Int                     (const unsigned int         pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Long                      (const long                 pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Long                    (const unsigned long        pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_LongLong                  (const long long            pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_U_LongLong                (const unsigned long long   pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Float                     (const float                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double                    (const double               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Char                  (const char*                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Char           (const char**               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Char                (const unsigned char*       pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Char         (const unsigned char**      pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Short                 (const short*               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Short          (const short**              pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Short               (const unsigned short*      pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Short        (const unsigned short**     pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Int                   (const int*                 pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Int            (const int**                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Int                 (const unsigned int*        pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Int          (const unsigned int**       pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Long                  (const long*                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Long           (const long**               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Long                (const unsigned long*       pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Long         (const unsigned long**      pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Long_Long             (const long long*           pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Long_Long      (const long long**          pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Long_Long           (const unsigned long long*  pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Long_Long    (const unsigned long long** pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Float                 (const float*               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Float          (const float**              pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Double                (const double*              pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Double         (const double**             pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Void                  (const void*                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Void           (const void**               pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Triple_Ptr_Void           (const void***              pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Bool                      (const bool                 pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Bool                  (const bool*                pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Bool           (const bool**               pValue);
                                    char                  __Object_UnBoxing_Char                    (const Object               pSelf);
                                    unsigned char         __Object_UnBoxing_U_Char                  (const Object               pSelf);
                                    short                 __Object_UnBoxing_Short                   (const Object               pSelf);
                                    unsigned short        __Object_UnBoxing_U_Short                 (const Object               pSelf);
                                    int                   __Object_UnBoxing_Int                     (const Object               pSelf);
                                    unsigned int          __Object_UnBoxing_U_Int                   (const Object               pSelf);
                                    long                  __Object_UnBoxing_Long                    (const Object               pSelf);
                                    unsigned long         __Object_UnBoxing_U_Long                  (const Object               pSelf);
                                    long long             __Object_UnBoxing_LongLong                (const Object               pSelf);
                                    unsigned long long    __Object_UnBoxing_U_LongLong              (const Object               pSelf);
                                    float                 __Object_UnBoxing_Float                   (const Object               pSelf);
                                    double                __Object_UnBoxing_Double                  (const Object               pSelf);
                                    char*                 __Object_UnBoxing_Ptr_Char                (const Object               pSelf);
                                    char**                __Object_UnBoxing_Double_Ptr_Char         (const Object               pSelf);
                                    unsigned char*        __Object_UnBoxing_Ptr_U_Char              (const Object               pSelf);
                                    unsigned char**       __Object_UnBoxing_Double_Ptr_U_Char       (const Object               pSelf);
                                    short*                __Object_UnBoxing_Ptr_Short               (const Object               pSelf);
                                    short**               __Object_UnBoxing_Double_Ptr_Short        (const Object               pSelf);
                                    unsigned short*       __Object_UnBoxing_Ptr_U_Short             (const Object               pSelf);
                                    unsigned short**      __Object_UnBoxing_Double_Ptr_U_Short      (const Object               pSelf);
                                    int*                  __Object_UnBoxing_Ptr_Int                 (const Object               pSelf);
                                    int**                 __Object_UnBoxing_Double_Ptr_Int          (const Object               pSelf);
                                    unsigned int*         __Object_UnBoxing_Ptr_U_Int               (const Object               pSelf);
                                    unsigned int**        __Object_UnBoxing_Double_Ptr_U_Int        (const Object               pSelf);
                                    long*                 __Object_UnBoxing_Ptr_Long                (const Object               pSelf);
                                    long**                __Object_UnBoxing_Double_Ptr_Long         (const Object               pSelf);
                                    unsigned long*        __Object_UnBoxing_Ptr_U_Long              (const Object               pSelf);
                                    unsigned long**       __Object_UnBoxing_Double_Ptr_U_Long       (const Object               pSelf);
                                    long long*            __Object_UnBoxing_Ptr_Long_Long           (const Object               pSelf);
                                    long long**           __Object_UnBoxing_Double_Ptr_Long_Long    (const Object               pSelf);
                                    unsigned long long*   __Object_UnBoxing_Ptr_U_Long_Long         (const Object               pSelf);
                                    unsigned long long**  __Object_UnBoxing_Double_Ptr_U_Long_Long  (const Object               pSelf);
                                    float*                __Object_UnBoxing_Ptr_Float               (const Object               pSelf);
                                    float**               __Object_UnBoxing_Double_Ptr_Float        (const Object               pSelf);
                                    double*               __Object_UnBoxing_Ptr_Double              (const Object               pSelf);
                                    double**              __Object_UnBoxing_Double_Ptr_Double       (const Object               pSelf);
                                    void*                 __Object_UnBoxing_Ptr_Void                (const Object               pSelf);
                                    void**                __Object_UnBoxing_Double_Ptr_Void         (const Object               pSelf);
                                    void***               __Object_UnBoxing_Triple_Ptr_Void         (const Object               pSelf);
                                    bool                  __Object_UnBoxing_Bool                    (const Object               pSelf);
                                    bool*                 __Object_UnBoxing_Ptr_Bool                (const Object               pSelf);
                                    bool**                __Object_UnBoxing_Double_Ptr_Bool         (const Object               pSelf);
// clang-format on

#define Object_GetData(DataType, Instance)                                     \
  _Generic((Instance), Object : *((DataType*)(Instance)->m_Value.m_Value1d))

// clang-format off
#define Obj(Instance)                                                          \
  _Generic((Instance),                                                         \
  default  : __SystemObject(Instance))                                         \
  (Instance)

#define Boxing(DataType) ((Object (*)(DataType)) __ObjectBoxingSearch(#DataType))
#define UnBoxing(DataType) ((DataType (*)(Object)) __ObjectUnBoxingSearch(#DataType))

bool Object_Compare(Object* Self, Object* Obj);
Func_t __ObjectBoxingSearch(const char *pDataType);
Func_t __ObjectUnBoxingSearch(const char *pDataType);

Object GetObject(const DataTypeInfo_t *pInfo, const void *pValue);
void   FreeObject(Object Ref);


#endif