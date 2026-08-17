
#include <GarbageCollection.h>
#include <Private_Json.h>
#include <Private_JsonAry.h>
#include <String.h>

bool
JSONAry_Clear(JSONAry pSelf)
{
  // 캐싱
  JSONAryNode node = pSelf->m_Nodes;
  JSONAryNode* Arys = MemoryCreate(sizeof(JSONAry_t) * pSelf->m_Length);
  Index_t i = 0;
  for (i = 0; i < pSelf->m_Length; i++, node = node->Next)
    Arys[i++] = node;

  // 각 원소 삭제

  for (i = 0; i < pSelf->m_Length; i++) {
    switch (Arys[i]->m_Value.m_DataType) {
      // 문자열 기반
      case JSONDataType_Decimal:
      case JSONDataType_Digit:
      case JSONDataType_Boolean:
      case JSONDataType_None:
        StringMethod.Destructor(Arys[i]->m_Value.m_Value.StringValue);
        MemoryRemove(Arys);
        break;
      // 참조 기반
      case JSONDataType_JSONObject:
        // TODO 이거 구현하려면 JSON에서 이 원소를 찾고, 그 필드이름을
        // 알아내야함. 즉 함수가 필요함.
        break;
      case JSONDataType_Ary:
        // TODO 이거 구현하려면 JSONAry에서 이 원소를 찾고, 그 원소 번호를
        // 알아내야함. 즉 함수가 필요함.
        break;
      // clang-format off
      default: break;
        // clang-format on
    }
  }

  pSelf->m_Length = 0;
  pSelf->m_Nodes = NULL;
  return false;
}
