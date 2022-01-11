
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

/**
 * @brief Src의 Length 만큼의 공간을 value로 초기화합니다.
 *
 * @param Src 수정할 주기억장치 상의 위치
 * @param value 덮어쓰는 값
 * @param WordSize 덮어쓰는 값의 크기
 * @param Length 수정할 메모리의 연속된 길이
 * @return None - 없음
 */
void
Private_MemorySet(void* Src, int value, Length WordSize, Length Length);

/**
 * @brief Src의 공간을 Data로 Length 만큼 복사합니다.
 *
 * @param Src 수정할 주기억장치 상의 위치
 * @param Data 덮어쓰는 값
 * @param Length 수정할 메모리의 크기
 * @return None - 없음
 */
void
Private_MemoryCopy(void* Src, void* Data, Length Length);

/**
 * @brief Src의 공간에 Data를 Length 복사합니다.
 *
 * @param Src 수정할 주기억장치 상의 위치
 * @param Data 덮어쓰는 값
 * @param Length 수정할 메모리의 크기
 * @return None - 없음
 */
void
Private_MemoryMove(void* Src, void* Data, Length Length);

/**
 * @brief Src와 Data의 Length 만큼 교환합니다.
 *
 * @param Src 교환할 주기억장치 상의 위치 1
 * @param Data 교환할 주기억장치 상의 위치 2
 * @param Length 교환할 메모리의 크기
 * @return None - 없음
 */
void
Private_MemorySwap(void* Src, void* Data, Length Length);

/**
 * @brief Obj1과 Obj2의 값을 비교합니다.
 *
 * @param Obj1 비교할 주기억장치 상의 위치 1
 * @param Obj2 비교할 주기억장치 상의 위치 2
 * @param Length 비교할 메모리의 크기
 * @return <true/false> 값이 같은 가?
 */
bool
Private_MemoryCompare(void* Obj1, void* Obj2, Length Length);

#endif