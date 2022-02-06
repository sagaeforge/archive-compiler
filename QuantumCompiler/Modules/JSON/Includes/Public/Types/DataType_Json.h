
#ifndef __PUBLIC_JSON_DATATYPE_JSON__
#define __PUBLIC_JSON_DATATYPE_JSON__

#include <Types/DataType_Object.h>
#include <Types/DataTypes_String.h>

typedef enum
{
  JSONDataType_Digit,
  JSONDataType_Decimal,
  JSONDataType_Boolean,
  JSONDataType_String,
  JSONDataType_JSONObject,
  JSONDataType_Ary,
  JSONDataType_NULL,
  JSONDataType_None
} JSONDataType;

// clang-format off
#pragma pack(push, 1)

typedef struct _JSONNode
{
  String              m_Name;
  union 
  {
    // * 기본 형식[type: null, digit, boolean, String] 만 저장함. 
    String StringValue;
    // * 참조 형식[type: JSONObject, Ary] 만 저장함. 
    void* ReferenceValue;
  } m_Value;
  JSONDataType        m_DataType;
  struct _JSONNode*   Next;
} JSONNode_t, *JSONNode;

typedef struct _JSONObject
{
  StringAry           m_FieldNames;
  Length_t            m_FieldLength;
  struct _JSONObject* m_Parent;
  JSONNode            m_Nodes;
} JSONObject_t, *JSONObject;

typedef struct _JSONAryNode {
  struct {
    union 
    {
      // * 기본 형식[type: null, digit, boolean, String] 만 저장함. 
      String StringValue;
      // * 참조 형식[type: JSONObject, Ary] 만 저장함. 
      void* ReferenceValue;
    } m_Value;
    JSONDataType        m_DataType;
  } m_Value;
  struct _JSONAryNode*  Next;
} JSONAryNode_t, *JSONAryNode;

typedef struct _JSONAry {
  struct {
    union
    {
      JSONObject        m_Object;
      struct _JSONAry*  m_Ary;
    };
    bool IsObject;
  } m_Parent;
  Length_t    m_Length;
  JSONAryNode m_Nodes;
} JSONAry_t, *JSONAry;


struct _JSONMethod {
  JSONObject    (*Constructor)            ();
  bool          (*Destructor)             (      JSONObject*);
  Object_t*     (*Get)                    (const JSONObject, const String);
  bool          (*Set)                    (      JSONObject, const String, const Object);
  bool          (*Append)                 (      JSONObject, const String, const Object);
  bool          (*Remove)                 (      JSONObject, const String);
  bool          (*TypeOf)                 (const JSONObject, const String, const JSONDataType);
  bool          (*FieldOf)                (const JSONObject, const String);
  bool          (*Compare)                (const JSONObject, const JSONObject);
  JSONDataType  (*Type)                   (const JSONObject, const String);
  StringAry     (*Export)                 (const JSONObject, const Length_t);
  JSONObject    (*Parent)                 (const JSONObject);
  bool          (*SetParent)              (      JSONObject, const JSONObject);
  Length_t      (*FiledLength)            (const JSONObject);
  JSONObject    (*Clone)                  (const JSONObject);
  bool          (*Print)                  (const JSONObject);
  bool          (*Clear)                  (      JSONObject);
};
struct _JSONAryMethod {
  JSONAry       (*Constructor)            ();
  bool          (*Destructor)             (      JSONAry*);
  Object_t*     (*Get)                    (const JSONAry, const Index_t);
  bool          (*Set)                    (      JSONAry, const Index_t, const Object);
  bool          (*Insert)                 (      JSONAry, const Index_t, const Object);
  bool          (*Remove)                 (      JSONAry, const Index_t);
  bool          (*Push)                   (      JSONAry, const Object);
  Object_t*     (*Pop)                    (      JSONAry);
  bool          (*Compare)                (const JSONAry, const JSONAry);
  bool          (*Contains)               (const JSONAry, const JSONAry);
  bool          (*Clear)                  (      JSONAry);
  bool          (*Object)                 (const JSONAry);
  bool          (*SetObject)              (const JSONAry, const JSONObject);
  Length_t      (*Length)                 (const JSONAry);
};

#pragma pack(pop)

#endif