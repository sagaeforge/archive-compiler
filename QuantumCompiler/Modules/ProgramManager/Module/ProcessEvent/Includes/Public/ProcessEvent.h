
#ifndef __PUBLIC_PROCESSEVENT__
#define __PUBLIC_PROCESSEVENT__

#include "Types/DataTypes_ProcessEvent.h"

/**
 * @brief 프로세스 이벤트 모듈 초기화
 *
 * @param None 없음
 * @return None - 없음
 */
void
ProcessEventModule_Initialized();

/**
 * @brief 업데이트 종료까지 대기
 *
 * @param Thread 업데이트 쓰레드
 * @return None - 없음
 */
void
Update_Wait(pthread_t* Thread);

/**
 * @brief 모든 업데이트 종료
 *
 * @param None 없음
 * @return None - 없음
 */
void
Update_AllStop();

#endif