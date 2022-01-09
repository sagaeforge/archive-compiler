
#ifndef __PUBLIC_DATATYPES_PROCESSEVENT__
#define __PUBLIC_DATATYPES_PROCESSEVENT__

#include "DataTypes.h"
#include "Delegate.h"
#include <pthread.h>

/** @brief 프로세스 이벤트 주기 */
typedef struct
{
  struct __FuncChainNode* Nodes;
  // clang-format off
  /**
   * @brief 프로세스 이벤트에 함수 추가
   * @param FP_Func - void (*) (void) 타입의 함수
   * @return None - 없음
   */
  void (*AddListener)       (FP_Func);
  /**
   * @brief 프로세스 이벤트에 함수 삭제
   * @param FP_Func - void (*) (void) 타입의 함수
   * @return None - 없음
   */
  void (*RemoveListener)    (FP_Func);
  /**
   * @brief 프로세스 이벤트에 등록된 모든 함수 삭제
   * @param None - 없음
   * @return None - 없음
   */
  void (*RemoveAllListener) ();
  /**
   * @brief 프로세스 이벤트 실행
   * @param None - 없음
   * @return None - 없음
   */
  void (*Invoke)            ();
  // clang-format on
} ProcessEvent;

/** @brief 프로세스 이벤트 코드 */
typedef enum
{
  /** @brief <ProcessEvent> 없음 */
  ProcessEvent_None = 0,
  /** @brief <ProcessEvent> 준비 단계 */
  ProcessEvent_Awake,
  /** @brief <ProcessEvent> 초기화 단계 */
  ProcessEvent_Init,
  /** @brief <ProcessEvent> 프로그램 시작 단계 */
  ProcessEvent_Start,
  /** @brief <ProcessEvent> 프로그램 루틴 실행 단계 */
  ProcessEvent_Main,
  /** @brief <ProcessEvent> 주기가 없는 업데이트 단계 */
  ProcessEvent_Update,
  /** @brief <ProcessEvent> 주기가 있는 업데이트 단계 */
  ProcessEvent_FixedUpdate,
  /** @brief <ProcessEvent> 프로그램 종료 단계 */
  ProcessEvent_Quit
} ProcessEventName;

#endif