
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_JsonAry.h>
#include <String.h>

bool
JSONAry_Remove(JSONAry pSelf, const Index_t pIndex)
{
  if (pSelf->m_Nodes == NULL) {
    Exception(ERROR, "원소가 없습니다. [size:%u]", pSelf->m_Length);
    return false;
  }

  pSelf->m_Length--;
  // 현재
  JSONAryNode node1 = pSelf->m_Nodes;
  // 과거나 미래
  JSONAryNode node2 = pSelf->m_Nodes->Next;

  if (pIndex == 0) {
    if (node1->m_Value.m_DataType == JSONDataType_Ary ||
        node1->m_Value.m_DataType == JSONDataType_JSONObject)
      MemoryRemove(node1->m_Value.m_Value.ReferenceValue);
    else
      StringMethod.Destructor(&node1->m_Value.m_Value.StringValue);
    MemoryRemove(node1);

    pSelf->m_Nodes = node2;
    return true;
  }

  // 노드 추출
  Length_t TargetIndex =
    pIndex >= pSelf->m_Length ? pSelf->m_Length - 1 : pIndex;

  Index_t i;
  for (i = 0; i < TargetIndex; i++) {
    node2 = node1;
    node1 = node1->Next;
  }

  if (node1->m_Value.m_DataType == JSONDataType_Ary ||
      node1->m_Value.m_DataType == JSONDataType_JSONObject)
    MemoryRemove(node1->m_Value.m_Value.ReferenceValue);
  else
    StringMethod.Destructor(&node1->m_Value.m_Value.StringValue);

  MemoryRemove(node1);

  pSelf->m_Nodes = node2;
  return true;
}
