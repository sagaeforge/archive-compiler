#ifndef __NLINKEDLISTS_H__
#define __NLINKEDLISTS_H__

#include <Types/DataType_Global.h>

typedef struct nListNode
{
  void* m_Value;
  struct nListNode* Next;
} nListNode_t, *nListNode_ptr;

typedef struct nList
{
  nListNode_ptr m_Nodes;
} nList_t, *nList_ptr;

#endif // __NLINKEDLISTS_H__