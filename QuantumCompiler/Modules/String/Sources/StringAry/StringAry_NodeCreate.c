
#include <GarbageCollection.h>
#include <Private_StringAry.h>

StringAryNode*
StringAry_NodeCreate()
{
  StringAryNode* node = (StringAryNode*)MemoryCreate(sizeof(StringAryNode));
  if (node == NULL)
    return NULL;
  node->Next = NULL;
  node->Value = NULL;
  return node;
}