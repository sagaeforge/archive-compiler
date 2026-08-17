
#include <GarbageCollection.h>
#include <Json.h>
#include <Private_Json.h>
#include <StringAry.h>

JSONAry
JSONAry_Constructor()
{
  JSONAry Ary = MemoryCreate(sizeof(JSONAry_t));
  Ary->m_Length = 0;
  Ary->m_Nodes = NULL;
  Ary->m_Parent.IsObject = true;
  Ary->m_Parent.m_Object = NULL;
  return Ary;
}