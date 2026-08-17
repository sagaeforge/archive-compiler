
#include <GarbageCollection.h>
#include <Private_Json.h>

JSONNode
JSON_NodeCreate()
{
  JSONNode node = MemoryCreate(sizeof(JSONNode_t));
  node->m_DataType = JSONDataType_None;
  node->m_Name = NULL;
  node->m_Value.StringValue = NULL;
  node->Next = NULL;
  return node;
}