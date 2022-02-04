
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <StringAry.h>

JSONObject
JSON_Constructor()
{
  JSONObject obj = MemoryCreate(sizeof(JSONObject_t));
  obj->m_FieldLength = 0;
  obj->m_FieldNames = StringAry(0);
  obj->m_Nodes = NULL;
  obj->m_Parent = NULL;
  return obj;
}

JSONObject
JSON_Constructor_Parent(JSONObject pParent)
{
  JSONObject obj = JSON_Constructor();
  obj->m_Parent = pParent;
  return obj;
}