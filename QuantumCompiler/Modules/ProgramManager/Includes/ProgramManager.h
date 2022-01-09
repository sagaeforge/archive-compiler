
#ifndef __PUBLIC_PROGRAMMANAGER__
#define __PUBLIC_PROGRAMMANAGER__

#include "DataTypes.h"

#include "Types/DataTypes_Exception.h"
#include "Types/DataTypes_GarbageCollection.h"
#include "Types/DataTypes_InputSystem.h"
#include "Types/DataTypes_ProcessEvent.h"

struct ProgramManager
{
  // [*] 프로세스 이벤트
  /** @brief 이벤트들 */
  ProcessEvent ProcessEvent[8];
  // [*] 가비지 컬렉션
  // 가비지 컬렉션
  struct
  {
    /**
     * @brief 메모리 생성
     * @param Length 생성할 크기
     * @return void* - 생성된 메모리 주소
     */
    void* (*Create)(Length);
    /**
     * @brief 수정이 불가능한 메모리 생성
     * @param Length 생성할 크기
     * @return void* - 생성된 메모리 주소
     */
    void* (*ConstCreate)(Length);
    /**
     * @brief 메모리 소멸
     * @param void** 생성된 메모리의 주소
     * @return None - 없음
     */
    void (*Remove)(void**);
    /**
     * @brief 메모리 설정
     * @param void* 수정할 메모리 주소
     * @param int 수정할 값
     * @param Length 수정할 값의 자료형 크기
     * @param Length 수정할 길이
     * @return None - 없음
     */
    void (*Set)(void*, int, Length, Length);
    /**
     * @brief 메모리 복사
     * @param void* 복사할 주소
     * @param void* 복사할 값
     * @param Length 복사할 길이
     * @return None - 없음
     */
    void (*Copy)(void*, void*, Length);
    /**
     * @brief 메모리 이동
     * @param void* 이동할 주소
     * @param void* 이동할 값
     * @param Length 이동할 길이
     * @return None - 없음
     */
    void (*Move)(void*, void*, Length);
    /**
     * @brief 메모리 교환
     * @param void* 교환할 주소.1
     * @param void* 교환할 주소.2
     * @param Length 교환할 주소의 크기
     * @return None - 없음
     */
    void (*Swap)(void*, void*, Length);
    /**
     * @brief 메모리 비교
     * @param void* 기준으로 사용할 값이 있는 주소
     * @param void* 비교할 값이 있는 주소
     * @param Length 비교할 크기
     * @return bool - 비교 여부
     */
    bool (*Compare)(void*, void*, Length);
    /**
     * @brief 메모리 길이
     * @param void * 찾을 값이 있는 주소
     * @return Length - 생성된 메모리의 길이
     */
    Length (*Length)(void*);
    /**
     * @brief 메모리 정보
     * @param Length 찾을 값이 있는 주소
     * @return MemoryInfo - 메모리의 대한 정보
     */
    MemoryInfo (*Info)(void*);
    /**
     * @brief 메모리 정보
     * @param Length 찾을 값이 있는 주소
     * @return MemoryInfo - 메모리의 대한 정보
     */
    bool (*Policy)(void*, MemoryPolicy);
    /**
     * @brief 메모리 정보
     * @param Length 찾을 값이 있는 주소
     * @return MemoryInfo - 메모리의 대한 정보
     */
    void (*PolicyAppend)(void*, MemoryPolicy);
    /**
     * @brief 메모리 정보
     * @param Length 찾을 값이 있는 주소
     * @return MemoryInfo - 메모리의 대한 정보
     */
    void (*PolicyRemove)(void*, MemoryPolicy);
  } GarbageCollection;

  // [*] 인풋 시스템
  /** @brief */
  struct
  {
    /** @brief */
    Input Input;
    /** @brief */
    Output Output;
    /** @brief */
    ErrorOutput Error;
  } InputSystem;

  // [*] 오류 처리
  // 오류 처리
  struct
  {
    ProgramError ErrorCode;
  } Exception;

  /**
   * @brief 프로그램 초기화
   * @param None 없음
   * @return None - 없음
   */
  void (*ProgramInit)();
  /**
   * @brief 프로그램 시작
   * @param None 없음
   * @return None - 없음
   */
  void (*ProgramStart)();
  /**
   * @brief 프로그램 종료
   * @param None 없음
   * @return None - 없음
   */
  void (*ProgramQuit)();
  /**
   * @brief 업데이트 시작
   * @param enum_ProcessEventCode 해당하는 업데이트 코드
   * @return None - 없음
   */
  void (*UpdateStart)(ProcessEventName);
  /**
   * @brief 업데이트 종료
   * @param enum_ProcessEventCode 해당하는 업데이트 코드
   * @return None - 없음
   */
  void (*UpdateStop)(ProcessEventName);
  /**
   * @brief 업데이트 쓰레드가 끝나기까지 대기 후 종료
   * @param enum_ProcessEventCode 해당하는 업데이트 코드
   * @return None - 없음
   */
  void (*UpdateStopWait)(ProcessEventName);

  /** @brief 내부에서 사용하는 멤버 **/
  struct
  {
    /** @brief 업데이트 주기용 CPU */
    pthread_t ProcessEvent_UpdateThread;
    /** @brief 고정된 업데이트 주기용 CPU */
    pthread_t ProcessEvent_FixedUpdateThread;

    /** @brief 고정된 업데이트 주기 */
    unsigned int ProcessEvent_FixedUpdateTime;

    /** @brief 프로그램 초기화 여부 */
    bool ProcessEvent_IsInitialized;
    /** @brief 프로그램 시작 여부 */
    bool ProcessEvent_IsStarted;

    /** @brief 업데이트 여부 */
    bool ProcessEvent_IsUpdated;
    /** @brief 고정된 업데이트 여부 */
    bool ProcessEvent_IsFixedUpdated;

    /** @brief 현재 가지고 있는 메모리의 총 개수 */
    Length GarbageCollection_UsedMemoryLength;
    /** @brief 현재 가지고 있는 메모리 페이지의 총 개수 */
    Length GarbageCollection_UsedMemoryPageLength;
    /** @brief 메모리 페이지 노드 */
    MemoryPage GarbageCollection_Pages;

    struct
    {
      void (*Update_Start)();
      void (*Update_Stop)();
      void (*Update_StopWait)();
      void (*FixedUpdate_Start)();
      void (*FixedUpdate_Stop)();
      void (*FixedUpdate_StopWait)();
    } Private_Method;

  } Member;
};

/**
 * @brief 프로그램 관리자 및 프로그램 지원 기능
 */
extern struct ProgramManager Application;

void
ProgramManager_Init();

#endif