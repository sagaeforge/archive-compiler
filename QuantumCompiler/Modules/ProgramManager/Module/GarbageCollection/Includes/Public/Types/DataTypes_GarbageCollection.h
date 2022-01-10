
#ifndef __PUBLIC_DATATYPES_GARBAGECOLLECTION__
#define __PUBLIC_DATATYPES_GARBAGECOLLECTION__

#include "DataTypes.h"

#define MemoryMaxLength 1024

#pragma pack(push, 1)

/** @brief  메모리 정책 */
typedef enum
{
  // clang-format off
  /** @brief  메모리 정책 없음 */
  MemoryPolicy_None             = 0,
  /** @brief  메모리 수정 불가 정책 */
  MemoryPolicy_Const            = (1 << 0),
  /** @brief  메모리 삭제 불가 정책 */
  MemoryPolicy_NoDestructor     = (1 << 1),
  /** @brief  메모리 시스템에서 사용하는 메모리 정책 */
  MemoryPolicy_SystemMemory     = (1 << 2)
  // clang-format on
} MemoryPolicy;

/** @brief GarbageCollection 상의 메모리 위치 */
typedef struct
{
  /** @brief 메모리 페이지 상의 메모리 위치 */
  Index PageIndex;
  /** @brief GarbageCollection 상의 메모리 페이지 위치 */
  Index MemoryIndex;
} MemoryPosition;

/** @brief 메모리 */
typedef struct
{
  /** @brief GarbageCollection 상의 메모리 위치 */
  void* Position;
  /** @brief 메모리 정책 */
  MemoryPolicy Policy;
  /** @brief 메모리가 차지하고 있는 크기 */
  Length Length;
} Memory;

/** @brief 메모리의 대한 정보 */
typedef struct
{
  /** @brief 메모리가 존재하는 가? */
  bool IsFound;
  /** @brief 메모리 */
  Memory Memory;
  /** @brief GarbageCollection 상의 메모리 위치 */
  MemoryPosition Position;
} MemoryInfo;

/** @brief 메모리 페이지 */
typedef struct _MemoryPage
{
  /** @brief 현재 사용하고 있는 메모리 크기 */
  Length UsedMemoryLength;
  /** @brief 메모리 저장 공간 */
  Memory Datas[MemoryMaxLength];
  /** @brief 페이지의 메모리 정보 */
  MemoryInfo Info;
  /** @brief 다음 페이지의 주기억장치 상의 위치 */
  struct _MemoryPage* Next;
} MemoryPage;

#pragma pack(pop)

#endif