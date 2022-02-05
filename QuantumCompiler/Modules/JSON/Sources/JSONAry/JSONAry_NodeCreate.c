
#include <GarbageCollection.h>
#include <Private_JsonAry.h>

JSONAryNode
JSONAry_NodeCreate()
{
  JSONAryNode node = MemoryCreate(sizeof(JSONAryNode_t));
  node->m_Value.m_DataType = JSONDataType_None;
  node->m_Value.m_Value.ReferenceValue = NULL;
  node->Next = NULL;
  return node;
}
