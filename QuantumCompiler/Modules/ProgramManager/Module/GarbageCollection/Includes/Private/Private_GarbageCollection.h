
#ifndef __PRIVATE_GARBAGECOLLECTION__
#define __PRIVATE_GARBAGECOLLECTION__

#include "GarbageCollection.h"

/**
 * @brief 메모리를 반환합니다.
 *
 * @param Position - 속한 GarbageCollection 위치
 * @return void* - 주기억장치상의 위치
 */
void*
GetMemory(MemoryPosition Position);

/**
 * @brief 메모리의 정보를 반환합니다.
 *
 * @param Obj - 주기억장치상의 위치
 * @return MemoryInfo - 해당하는 메모리의 정보
 */
MemoryInfo
Info(void* Obj);

/**
 * @brief GarbageCollection에 Obj를 등록합니다.
 *
 * @param Obj 주기억장치 상의 위치
 * @param Length 해당 메모리의 주기억장치 사용량
 * @return None - 없음
 */
void
GC_Append(void* Obj, Length Length);

/**
 * @brief GarbageCollection에 등록된 Obj를 삭제합니다.
 *
 * @param Obj 주기억장치 상의 위치
 * @return None - 없음
 */
void
GC_Remove(void* Obj);

/**
 * @brief 메모리(Obj)의 메모리 정책이 Policy와 맞는가?
 *
 * @param Obj 주기억장치 상의 위치
 * @param Policy 정책
 * @return <true/false> - 여부
 */
bool
Policy(void* Obj, MemoryPolicy Policy);

/**
 * @brief 메모리(Obj)의 메모리 정책(Policy)를 추가합니다.
 *
 * @param Obj 주기억장치 상의 위치
 * @param Policy 정책
 * @return None - 없음
 */
void
Policy_Append(void* Obj, MemoryPolicy Policy);

/**
 * @brief 메모리(Obj)의 메모리 정책(Policy)를 삭제합니다.
 *
 * @param Obj 주기억장치 상의 위치
 * @param Policy 정책
 * @return None - 없음
 */
void
Policy_Remove(void* Obj, MemoryPolicy Policy);

/**
 * @brief GarbageCollection 안에 있는 Index 번째 페이지를 가져옵니다.
 *
 * @param Index 인덱스
 * @return MemoryPage* - 메모리 페이지
 */
MemoryPage*
PageGet(Index Index);

/**
 * @brief GarbageCollection 안에 있는 여유 공간이 있는 페이지를 가져옵니다.
 *
 * @param None 없음
 * @return MemoryPage* - 메모리 페이지
 */
MemoryPage*
EmptyPageGet();

#endif