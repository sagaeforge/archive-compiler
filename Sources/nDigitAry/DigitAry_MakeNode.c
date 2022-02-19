
#include <stdio.h>
#include <stdlib.h>

#include <Exception.h>
#include <Module/nDigitAry.h>

nDigitAryNode_ptr
DigitAry_MakeNode()
{
  nDigitAryNode_ptr _Node = malloc(sizeof(nDigitAryNode_t));
  if (!_Node) {
    Exception(ERROR, "nDigitAryNode를 생성할 수 없습니다.");
    return NULL;
  }

  _Node->m_Value = 0;
  _Node->Next = NULL;
  return _Node;
}