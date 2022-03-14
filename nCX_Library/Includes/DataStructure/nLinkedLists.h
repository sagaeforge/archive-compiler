#ifndef __NLINKEDLISTS_H__
#define __NLINKEDLISTS_H__

#include <Types/DataType_Global.h>

#pragma pack(push, 1)

typedef struct nListNode
{
  void* m_Value;
  struct nListNode* Next;
} nListNode_t, *nListNode_ptr;

typedef struct nList
{
  Index_t m_Length;
  nListNode_ptr m_Nodes;
} nList_t, *nList_ptr;

#pragma pack(pop)
#endif // __NLINKEDLISTS_H__