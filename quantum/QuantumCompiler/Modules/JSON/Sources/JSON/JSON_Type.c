
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_Json.h>
#include <String.h>
#include <StringLib.h>

JSONDataType
JSON_Type(const JSONObject pSelf, const String pFieldName)
{
  JSONNode node = pSelf->m_Nodes;

  Index_t i;
  for (i = 0; i < pSelf->m_FieldLength; i++) {
    if (StringMethod.Compare(node->m_Name, pFieldName)) {
      return node->m_DataType;
    }
    node = node->Next;
  }

  Exception(ERROR, "해당 필드가 없습니다. [field:%S]", pFieldName->m_Value);
  return JSONDataType_None;
}
