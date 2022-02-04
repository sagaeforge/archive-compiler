
#ifndef __PUBLIC_JSON_DATATYPE_JSON__
#define __PUBLIC_JSON_DATATYPE_JSON__

#include <Object.h>
#include <String.h>

typedef enum
{
  JSONDataType_Digit,
  JSONDataType_Decimal,
  JSONDataType_Boolean,
  JSONDataType_String,
  JSONDataType_JSONObject,
  JSONDataType_Digit_Ary,
  JSONDataType_Decimal_Ary,
  JSONDataType_Boolean_Ary,
  JSONDataType_String_Ary,
  JSONDataType_JSONObject_Ary,
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
  Length_t            m_Length;
  String              m_AryInfo;
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

struct JSONMethod {
  JSONObject    (*Constructor)               ();
  bool          (*Destructor)                (const JSONObject *);
  bool          (*Read_Str)                  (JSONObject,const String);
  bool          (*Read_StrAry)               (JSONObject,const StringAry);
  bool          (*Read_File)                 (JSONObject,const FILE*);
  bool          (*Write_Str)                 (String*, const JSONObject);
  bool          (*Write_StrAry)              (StringAry*, const JSONObject);
  bool          (*Write_File)                (FILE *,  const JSONObject);
  Object        (*Get)                       (const JSONObject, const String);
  int64_t       (*GetDeciaml)                (const JSONObject, const String);
  double        (*GetDigit)                  (const JSONObject, const String);
  bool          (*GetBool)                   (const JSONObject, const String);
  String_t     *(*GetString)                 (const JSONObject, const String);
  JSONObject    (*GetObject)                 (const JSONObject, const String);
  void*         (*GetAry)                    (const JSONObject, const String);
  void*         (*GetNULL)                   (const JSONObject, const String);
  bool          (*Set)                       (JSONObject, const String, const Object);
  bool          (*SetDeciaml)                (JSONObject, const String, const int64_t);
  bool          (*SetDigit)                  (JSONObject, const String, const double);
  bool          (*SetBool)                   (JSONObject, const String, const bool);
  bool          (*SetString)                 (JSONObject, const String, const String);
  bool          (*SetObject)                 (JSONObject, const String, const JSONObject);
  bool          (*SetAry)                    (JSONObject, const String, const void*);
  bool          (*SetNULL)                   (JSONObject, const String);
  bool          (*Append)                    (JSONObject, const String, const Object);
  bool          (*AppendDeciaml)             (JSONObject, const String, const int64_t);
  bool          (*AppendDigit)               (JSONObject, const String, const double);
  bool          (*AppendBool)                (JSONObject, const String, const bool);
  bool          (*AppendString)              (JSONObject, const String, const String);
  bool          (*AppendObject)              (JSONObject, const String, const JSONObject);
  bool          (*AppendAry)                 (JSONObject, const String, const void*);
  bool          (*AppendNULL)                (JSONObject, const String);
  bool          (*AppendDeciamlAry)          (JSONObject, const String, const int64_t*);
  bool          (*AppendDigitAry)            (JSONObject, const String, const double*);
  bool          (*AppendBoolAry)             (JSONObject, const String, const bool*);
  bool          (*AppendStringAry)           (JSONObject, const String, const StringAry);
  bool          (*AppendObjectAry)           (JSONObject, const String, const JSONObject);
  bool          (*Remove)                    (JSONObject, const String);
  bool          (*RemoveAryAt)               (JSONObject, const Index_t);
  bool          (*IsTypeOf)                  (const JSONObject, const String, const JSONDataType);
  bool          (*IsFieldOf)                 (const JSONObject, const String);
  bool          (*Compare)                   (const JSONObject, const JSONObject);
  JSONDataType  (*GetType)                   (const JSONObject, const String);
  String_t*     (*GetAryType)                (const JSONObject, const String);
  bool          (*Contains)                  (const JSONObject, const String);
  JSONObject    (*Sum)                       (const JSONObject, const JSONObject, const int);
  StringAry     (*Export)                    (const JSONObject, const Length_t);
  JSONObject    (*GetParent)                 (const JSONObject);
  Length_t      (*GetFiledLength)            (const JSONObject);
  JSONObject    (*Clone)                     (const JSONObject);
  bool          (*Print)                     (const JSONObject);
  bool          (*Clear)                     (const JSONObject);
};

#pragma pack(pop)

#endif