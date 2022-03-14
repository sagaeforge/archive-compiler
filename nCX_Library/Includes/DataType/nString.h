#ifndef __DATATYPE_NSTRING_H__
#define __DATATYPE_NSTRING_H__

#include <DataType/Global.h>

#include <DataStructure/DataType/nList.h>

#pragma pack(push, 1)

typedef char *Chs_t;
typedef wchar_t *Wcs_t;

typedef struct {
  Wcs_t m_Value;
  Length_t m_Length;
} nString_t, *nString_ptr;

typedef struct nStringAryNode {
  nString_ptr m_Value;
  struct nStringAryNode *Next;
} nStringAryNode_t, *nStringAryNode_ptr;

typedef struct {
  Length_t m_Length;
  nLinkedList_ptr m_Nodes;
} nStringAry_t, *nStringAry_ptr;

typedef enum {
  k_nRegExpFlag_None = 0,
  k_nRegExpFlag_Global = (1 << 0),
  k_nRegExpFlag_HasIndices = (1 << 1),
  k_nRegExpFlag_IgnoreCase = (1 << 2),
  k_nRegExpFlag_Multiline = (1 << 3),
  k_nRegExpFlag_DotAll = (1 << 4),
  k_nRegExpFlag_Unicode = (1 << 5),
  k_nRegExpFlag_Sticky = (1 << 6)
} nRegExpFlag_t,
    nRegExpFlag_enum;

typedef struct {
  nRegExpFlag_t m_Flag;
  nString_ptr m_Pattern;
} nRegExp_t, *nRegExp_ptr;

typedef struct nRegExpResultNode {
  Index_t StartIndex;
  Index_t LastIndex;
  struct nRegExpResultNode *Next;
} nRegExpResultNode_t, *nRegExpResultNode_ptr;

typedef struct {
  nString_ptr m_OrignalText;
  nRegExp_ptr m_RegExp;
  bool isFound;
  struct {
    Length_t m_Count;
    nRegExpResultNode_ptr m_Nodes;
  } m_Result;
} nRegExpResult_t, *nRegExpResult_ptr;

#pragma pack(pop)

#endif // __DATATYPE_NSTRING_H__