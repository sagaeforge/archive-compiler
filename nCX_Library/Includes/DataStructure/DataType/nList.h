#ifndef __DATATYPE_NLIST_H__
#define __DATATYPE_NLIST_H__

#include <DataType/Global.h>

#pragma pack(push, 1)
typedef struct nLinkedListNode {
  void *m_Value;
  struct nLinkedListNode *Next;
  struct nLinkedListNode *Prev;
} nLinkedListNode_t, *nLinkedListNode_ptr;

typedef struct {
  Length_t m_Length;
  nLinkedListNode_ptr m_Nodes;
  Index_t m_LastAccessIndex;
  nLinkedListNode_ptr m_LastAccessNode;
} nLinkedList_t, *nLinkedList_ptr;
#pragma pack(pop)
#endif // __NLISTS_H__