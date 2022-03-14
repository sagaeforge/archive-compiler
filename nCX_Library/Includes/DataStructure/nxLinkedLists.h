#ifndef __NXLIKEDLISTS_H__
#define __NXLIKEDLISTS_H__

#include <Types/DataType_Global.h>
#include <nObject.h>

typedef struct nxListNode
{
  Object_ptr m_Value;
  struct nxListNode* Next;
} nxListNode_t, *nxListNode_ptr;

typedef struct nxList
{
  Index_t m_Length;
  nxListNode_ptr m_Nodes;
} nxList_t, *nxList_ptr;

#endif // __NXLIKEDLISTS_H__