
#include "Object.h"
#include "Application.h"

Func_t
__ObjectBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    // TODO Exception 처리
    return NULL;
  if (Info->m_Boxing != NULL)
    return Info->m_Boxing;
  // TODO Exception 처리
  // 박싱 함수가 없을 때
  return NULL;
}

Func_t
__ObjectUnBoxingSearch(const char* pDataType)
{
  const DataTypeInfo_t* Info = DataType_Find(pDataType);
  if (Info == NULL)
    // TODO Exception 처리
    return NULL;
  if (Info->m_UnBoxing != NULL)
    return Info->m_UnBoxing;
  // TODO Exception 처리
  // 언박싱 함수가 없을 때
  return NULL;
}

// clang-format off
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Char                      (const char                   pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Char                    (const unsigned char          pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Short                     (const short                  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Short                   (const unsigned short         pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Int                       (const int                    pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Int                     (const unsigned int           pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Long                      (const long                   pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_U_Long                    (const unsigned long          pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_LongLong                  (const long long              pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_U_LongLong                (const unsigned long long     pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Float                     (const float                  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double                    (const double                 pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Char                  (const char*                  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Char           (const char**                 pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Char                (const unsigned char*         pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Char         (const unsigned char**        pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Short                 (const short*                 pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Short          (const short**                pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Short               (const unsigned short*        pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Short        (const unsigned short**       pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Int                   (const int*                   pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Int            (const int**                  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Int                 (const unsigned int*          pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Int          (const unsigned int**         pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Long                  (const long*                  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Long           (const long**                 pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Long                (const unsigned long*         pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Long         (const unsigned long**        pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Long_Long             (const long long *            pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Long_Long      (const long long **           pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_U_Long_Long           (const unsigned long long *   pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_U_Long_Long    (const unsigned long long **  pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Float                 (const float *                pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Float          (const float **               pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Double                (const double *               pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Double         (const double **              pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Ptr_Void                  (const void *                 pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Double_Ptr_Void           (const void **                pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
// __attribute__((warn_unused_result)) const Object          __Object_Boxing_Triple_Ptr_Void           (const void ***               pValue) { /* TODO GC 기능 만들어지면 하셈*/ return NULL; }
                                    char                  __Object_UnBoxing_Char                    (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    unsigned char         __Object_UnBoxing_U_Char                  (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    short                 __Object_UnBoxing_Short                   (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    unsigned short        __Object_UnBoxing_U_Short                 (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    // int                   __Object_UnBoxing_Int                     (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    unsigned int          __Object_UnBoxing_U_Int                   (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    long                  __Object_UnBoxing_Long                    (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    unsigned long         __Object_UnBoxing_U_Long                  (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    long long             __Object_UnBoxing_LongLong                (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    unsigned long long    __Object_UnBoxing_U_LongLong              (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    float                 __Object_UnBoxing_Float                   (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    double                __Object_UnBoxing_Double                  (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return 0; }
                                    char*                 __Object_UnBoxing_Ptr_Char                (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    char**                __Object_UnBoxing_Double_Ptr_Char         (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned char*        __Object_UnBoxing_Ptr_U_Char              (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned char**       __Object_UnBoxing_Double_Ptr_U_Char       (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    short*                __Object_UnBoxing_Ptr_Short               (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    short**               __Object_UnBoxing_Double_Ptr_Short        (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned short*       __Object_UnBoxing_Ptr_U_Short             (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned short**      __Object_UnBoxing_Double_Ptr_U_Short      (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    int*                  __Object_UnBoxing_Ptr_Int                 (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    int**                 __Object_UnBoxing_Double_Ptr_Int          (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned int*         __Object_UnBoxing_Ptr_U_Int               (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned int**        __Object_UnBoxing_Double_Ptr_U_Int        (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    long*                 __Object_UnBoxing_Ptr_Long                (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    long**                __Object_UnBoxing_Double_Ptr_Long         (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned long*        __Object_UnBoxing_Ptr_U_Long              (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned long**       __Object_UnBoxing_Double_Ptr_U_Long       (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    long long *           __Object_UnBoxing_Ptr_Long_Long           (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    long long **          __Object_UnBoxing_Double_Ptr_Long_Long    (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned long long *  __Object_UnBoxing_Ptr_U_Long_Long         (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    unsigned long long ** __Object_UnBoxing_Double_Ptr_U_Long_Long  (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    float *               __Object_UnBoxing_Ptr_Float               (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    float **              __Object_UnBoxing_Double_Ptr_Float        (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    double *              __Object_UnBoxing_Ptr_Double              (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    double **             __Object_UnBoxing_Double_Ptr_Double       (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    void *                __Object_UnBoxing_Ptr_Void                (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    void **               __Object_UnBoxing_Double_Ptr_Void         (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
                                    void ***              __Object_UnBoxing_Triple_Ptr_Void         (const Object                 pSelf)  { /* TODO GC 기능이 만들어자면 하셈 */ return NULL; }
