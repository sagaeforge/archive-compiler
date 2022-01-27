
#ifndef __PUBLIC_APPLICATION_DATATYPE_PROCESSEVENT__
#define __PUBLIC_APPLICATION_DATATYPE_PROCESSEVENT__

#pragma pack(push, 1)
// clang-format off

#include <Types/DataType.h>
#include <Delegate.h>

/** @brief 프로세스 이벤트 주기 */
typedef struct
{
  struct __FuncChainNode* m_Nodes;
  // clang-format off
  /**
   * @brief 프로세스 이벤트에 함수 추가
   * @param Func_t - void (*) (void) 타입의 함수
   * @return None - 없음
   */
  void (*AddListener)       (Func_t);
  /**
   * @brief 프로세스 이벤트에 함수 삭제
   * @param Func_t - void (*) (void) 타입의 함수
   * @return None - 없음
   */
  void (*RemoveListener)    (Func_t);
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
  /** @brief <ProcessEvent> 준비 단계 */
  ProcessEvent_Awake = 0,
  /** @brief <ProcessEvent> 초기화 단계 */
  ProcessEvent_Init,
  /** @brief <ProcessEvent> 프로그램 시작 단계 */
  ProcessEvent_Start,
  /** @brief <ProcessEvent> 프로그램 실행 단계 */
  ProcessEvent_Main,
  /** @brief <ProcessEvent> 프로그램 종료 단계 */
  ProcessEvent_Quit,
  /** @brief <ProcessEvent> 주기가 없는 업데이트 단계 */
  ProcessEvent_Update,
  /** @brief <ProcessEvent> 주기가 있는 업데이트 단계 */
  ProcessEvent_FixedUpdate,
  /** @brief <ProcessEvent> 없음 */
  ProcessEvent_None = 7
} ProcessEventName;

// clang-format on
#pragma pack(pop)

#endif