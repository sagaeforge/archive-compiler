#ifndef __DATATYPE_NSTACK_H__
#define __DATATYPE_NSTACK_H__

#include <DataType/Global.h>

#pragma pack(push, 1)
typedef struct nStackNode {
  void *m_Value;
  struct nStackNode *Next;
  struct nStackNode *Prev;
} nStackNode_t, *nStackNode_ptr;

typedef struct {
  Length_t m_Length;
  nStackNode_ptr m_Top;
} nStack_t, *nStack_ptr;

#pragma pack(pop)

#endif // __NSTACK_H__