#ifndef __GARBAGECOLLECTION__
#define __GARBAGECOLLECTION__

#include "DataTypes.h"
#include "Types/DataTypes_GarbageCollection.h"

/**
 * @brief Length 만큼 메모리를 생성합니다.
 *
 * @param Length 생성할 크기
 * @return void* - 생성된 주기억장치 상의 위치
 */
void*
MemoryCreate(Length Length);

/**
 * @brief Length 만큼 수정할 수 없는 메모리를 생성합니다.
 *
 * @param Length 생성할 크기
 * @return void* - 생성된 주기억장치 상의 위치
 */
void*
MemoryConstCreate(Length Length);

/**
 * @brief 주기억장치 상의 존재하는 ptr의 공간을 운영체제에 반환합니다.
 *
 * @param ptr 반환할 주기억장치 상의 위치
 * @return None - 없음
 */
void
MemoryRemove(void** ptr);

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
MemorySet(void* Src, int value, Length WordSize, Length Length);

/**
 * @brief Src의 공간을 Data로 Length 만큼 복사합니다.
 *
 * @param Src 수정할 주기억장치 상의 위치
 * @param Data 덮어쓰는 값
 * @param Length 수정할 메모리의 크기
 * @return None - 없음
 */
void
MemoryCopy(void* Src, void* Data, Length Length);

/**
 * @brief Src의 공간에 Data를 Length 복사합니다.
 *
 * @param Src 수정할 주기억장치 상의 위치
 * @param Data 덮어쓰는 값
 * @param Length 수정할 메모리의 크기
 * @return None - 없음
 */
void
MemoryMove(void* Src, void* Data, Length Length);

/**
 * @brief Src와 Data의 Length 만큼 교환합니다.
 *
 * @param Src 교환할 주기억장치 상의 위치 1
 * @param Data 교환할 주기억장치 상의 위치 2
 * @param Length 교환할 메모리의 크기
 * @return None - 없음
 */
void
MemorySwap(void* Src, void* Data, Length Length);

/**
 * @brief Obj1과 Obj2의 값을 비교합니다.
 *
 * @param Obj1 비교할 주기억장치 상의 위치 1
 * @param Obj2 비교할 주기억장치 상의 위치 2
 * @param Length 비교할 메모리의 크기
 * @return <true/false> 값이 같은 가?
 */
bool
MemoryCompare(void* Obj1, void* Obj2, Length Length);

/**
 * @brief Obj의 메모리 크기를 반환합니다.
 *
 * @param Obj 주기억장치 상의 위치
 * @return Length 주기억장치에서 사용하고 있는 메모리 크기
 */
Length
MemoryLength(void* Obj);

/**
 * @brief GarbageCollectionModule을 초기화합니다.
 *
 * @param None 없음
 * @return None - 없음
 */
void
GarbageCollectionModule_Initialized();

#endif