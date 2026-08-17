
#include <GarbageCollection.h>
#include <Private_Json.h>
#include <Private_JsonAry.h>
#include <String.h>

bool
JSONAry_Compare(const JSONAry pSelf, const JSONAry pTarget)
{
  if (pSelf->m_Length != pTarget->m_Length)
    return false;

  // 캐싱
  JSONAryNode node = pSelf->m_Nodes;
  JSONAryNode* Arys = MemoryCreate(sizeof(JSONAry_t) * pSelf->m_Length);
  Index_t i = 0;
  for (i = 0; i < pSelf->m_Length; i++, node = node->Next)
    Arys[i++] = node;

  // 비교
  node = pTarget->m_Nodes;
  for (i = 0; i < pTarget->m_Length; i++, node = node->Next) {
    if (Arys[i]->m_Value.m_DataType != node->m_Value.m_DataType)
      return false;

    switch (Arys[i]->m_Value.m_DataType) {
      // 문자열 기반
      case JSONDataType_Decimal:
      case JSONDataType_Digit:
      case JSONDataType_Boolean:
      case JSONDataType_None:
        if (!StringMethod.Compare(Arys[i]->m_Value.m_Value.StringValue,
                                  node->m_Value.m_Value.StringValue)) {
          MemoryRemove(Arys);
          return false;
        }
        break;
      // 참조 기반
      case JSONDataType_JSONObject:
        if (!JSON_Compare(Arys[i]->m_Value.m_Value.ReferenceValue,
                          node->m_Value.m_Value.ReferenceValue)) {
          MemoryRemove(Arys);
          return false;
        }
        break;
      case JSONDataType_Ary:
        if (!JSONAry_Compare(Arys[i]->m_Value.m_Value.ReferenceValue,
                             node->m_Value.m_Value.ReferenceValue)) {
          MemoryRemove(Arys);
          return false;
        }
        break;
      // clang-format off
      default: break;
        // clang-format on
    }
  }

  MemoryRemove(Arys);
  return true;
}
