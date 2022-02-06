
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_Json.h>
#include <String.h>
#include <StringLib.h>

bool
JSON_TypeOf(const JSONObject pSelf,
            const String pFieldName,
            const JSONDataType pType)
{
  JSONNode node = pSelf->m_Nodes;

  Index_t i;
  for (i = 0; i < pSelf->m_FieldLength; i++) {
    if (StringMethod.Compare(node->m_Name, pFieldName)) {
      if (node->m_DataType == pType)
        return true;
      return false;
    }
    node = node->Next;
  }

  Exception(ERROR, "해당 필드가 없습니다. [field:%S]", pFieldName->m_Value);
  return false;
}
