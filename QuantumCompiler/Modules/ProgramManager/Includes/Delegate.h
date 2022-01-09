#ifndef __DELEGATE__
#define __DELEGATE__

#include <stdarg.h>
#include <stdio.h>

/**
 * [*] 델리게이트
 * [+ START] 델리게이트 정의
 * * 반환 타입이 있으며 매개 변수가 존재하는 경우
 */
#define Define_Delegate(Ret, Name, args...)                                    \
  typedef Ret (*Delegate_##Name)(args);                                        \
  struct __DelegateChainNode_##Name                                            \
  {                                                                            \
    Delegate_##Name Method;                                                    \
    struct __DelegateChainNode_##Name* Next;                                   \
  };                                                                           \
  typedef struct __DelegateChain_##Name                                        \
  {                                                                            \
    __DelegateChainNode_##Name* Nodes;                                         \
    void (*AddListener)(struct __DelegateChain_##Name*, Delegate_##Name);      \
    void (*RemoveListener)(struct __DelegateChain_##Name*, Delegate_##Name);   \
    void (*RemoveAllListener)(struct __DelegateChain_##Name*);                 \
    void (*Invoke)(struct __DelegateChain_##Name*, args);                      \
  } DelegateChain_##Name
// [+ End] 델리게이트 정의 끝

// [+ START] 델리게이트
#define Delegate(Ret, Name, args...)                                           \
  Define_Delegate(Ret, Name, args);                                            \
  DelegateChain_##Name Name
// [+ End] 델리게이트 끝

/**
 * [*] 액션 정의
 * [+ START] 액션 정의
 * * 반환 타입이 없으며 매개 변수가 존재하는 경우
 */
#define Define_Action(Name, args...)                                           \
  typedef void (*Action_##Name)(args);                                         \
  struct __ActionChainNode_##Name                                              \
  {                                                                            \
    Action_##Name Method;                                                      \
    struct __ActionChainNode_##Name* Next;                                     \
  };                                                                           \
  typedef struct __ActionChain_##Name                                          \
  {                                                                            \
    __ActionChainNode_##Name* Nodes;                                           \
    void (*AddListener)(struct __ActionChain_##Name*, Action_##Name);          \
    void (*RemoveListener)(struct __ActionChain_##Name*, Action_##Name);       \
    void (*RemoveAllListener)(struct __ActionChain_##Name*);                   \
    void (*Invoke)(struct __ActionChain_##Name*, args);                        \
  } ActionChain_##Name
// [+ End] 액션 정의 끝

// [+ START] 액션
#define Action(Name, args...)                                                  \
  Define_Action(Name, args);                                                   \
  ActionChain_##Name Name
// [+ End] 액션 끝

/**
 * [*] 펑크 정의
 * [+ START] 펑크 정의
 * * 반환 타입이 없으며 매개 변수도 존재하지 않는 경우
 */
typedef void (*FP_Func)();
typedef struct __FuncChainNode
{
  FP_Func Callback;
  struct __FuncChainNode* Next;
} FuncChainNode;
typedef struct __FuncChain
{
  FuncChainNode* Nodes;
  void (*AddListener)(struct __FuncChain*, FP_Func);
  void (*RemoveListener)(struct __FuncChain*, FP_Func);
  void (*RemoveAllListener)(struct __FuncChain*);
  void (*Invoke)(struct __FuncChain*);
} FuncChain;
// [+ End] 펑크 끝

void
FuncChain_Setting(FuncChain* Chain);

#endif