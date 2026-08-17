
#include <stdlib.h>

#include <Module/nStringAry.h>

nStringAryNode_ptr
StringAry_MakeNode()
{
  nStringAryNode_ptr _Node = (nStringAryNode_ptr)malloc(sizeof(nStringAryNode_t));
  _Node->m_Value = NULL;
  _Node->Next = NULL;
  return _Node;
}
