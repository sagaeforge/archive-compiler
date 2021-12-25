#ifndef __PROGRAM_MANAGER__
#define __PROGRAM_MANAGER__

#include "DataTypes.h"
#include "Delegate.h"

#define MemoryMaxLength 32

#pragma pack(push, 1)

// clang-format off
typedef enum {
  MemoryPolicey_None             = 0,
  MemoryPolicey_Const            = (1 << 0),
  MemoryPolicey_NotMemoryRemove  = (1 << 1),
  MemoryPolicey_NotMemorySet     = (1 << 2),
  MemoryPolicey_NotMemoryCopy    = (1 << 3),
  MemoryPolicey_NotMemoryMove    = (1 << 4),
  MemoryPolicey_NotMemorySwap    = (1 << 5),
  MemoryPolicey_NotMemoryCompare = (1 << 6),
  MemoryPolicey_NotMemoryLength  = (1 << 7),
} MemoryPolicey;
// clang-format on

typedef struct {
  Index PageIndex;
  Index MemoryIndex;
} MemoryPosition;

typedef struct {
  bool IsFounded;
  void *Value;
  Length Length;
  MemoryPosition Position;
  MemoryPolicey Policy;
} MemoryInfo;

/** @brief 메모리 페이지 */
typedef struct _MemoryPage {
  /** @brief 사용중인 메모리 크기 */
  Length UsedMemoryLength;
  /** @brief 사용중인 메모리 정보 집합 */
  struct {
    /** @brief 사용중인 메모리 위치 */
    void *Value;
    /** @brief 사용중인 메모리 크기 */
    Length Length;
    /** @brief 사용중인 메모리 정책 */
    MemoryPolicey Policey;
  } Datas[MemoryMaxLength];
  /** @brief 다음 메모리 정보 집합 */
  struct _MemoryPage *Next;
} MemoryPage;

/** @brief 프로세스 이벤트 체인 */
typedef struct {
  /** @brief 프로세스 이벤트 노드들 */
  struct __FuncChainNode *Nodes;
  // clang-format off
  void (*AddListener)       (FP_Func);
  void (*RemoveListener)    (FP_Func);
  void (*RemoveAllListener) ();
  void (*Invoke)            ();
  // clang-format on
} ProcessEvent;

struct ProgramManager {
  /** @brief 준비 단계 */
  ProcessEvent Awake;
  /** @brief 초기화 단계 */
  ProcessEvent Init;
  /** @brief 실행 단계 */
  ProcessEvent Start;
  /** @brief 종료 단계 */
  ProcessEvent Quit;

  struct {
    /** @brief 종료 단계 */
    Length UsedMemoryLength;
    /** @brief 종료 단계 */
    Length UsedMemoryPageLength;

    /** @brief 종료 단계 */
    MemoryPage Pages;

    /** @brief 종료 단계 */
    struct {
      // clang-format off
      void *(*MemoryCreate)       (Length);
      void *(*MemoryConstCreate)  (Length);
      void  (*MemoryRemove)       (void **);
      void  (*MemorySet)          (void *, int, Length, Length);
      void  (*MemoryCopy)         (void *, void *, Length);
      void  (*MemoryMove)         (void *, void *, Length);
      void  (*MemorySwap)         (void *, void *, Length);
      bool  (*MemoryCompare)      (void *, void *, Length);
      Length (*MemoryLength)      (void *);
      
      void  (*Clear)              ();
      void *(*Memory)             (MemoryPosition);
      MemoryInfo (*Info)          (void *);
      bool (*Policy)              (void *, MemoryPolicey);
      void (*PolicyAppend)        (void *, MemoryPolicey);
      void (*PolicyRemove)        (void *, MemoryPolicey);
      // clang-format on
    } Method;
  } GarbageCollection;
  /** @brief 종료 단계 */
  struct {
    // clang-format off
    void (*ProgramStart)();
    void (*ProgramQuit) ();
    // clang-format on
  } Method;
};
#pragma pack(pop)
extern struct ProgramManager Manager;

void ProgramManager_Init();

#endif