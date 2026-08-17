#ifndef __DATATYPE_NQUEUE_H__
#define __DATATYPE_NQUEUE_H__

#include <DataType/Global.h>

#pragma pack(push, 1)
typedef struct nQueueNode {
  void *m_Value;
  struct nQueueNode *Next;
} nQueueNode_t, *nQueueNode_ptr;

typedef struct {
  Length_t m_Length;
  nQueueNode_ptr m_Front;
  nQueueNode_ptr m_Rear;
} nQueue_t, *nQueue_ptr;

#pragma pack(pop)

#endif // __NQUEUE_H__