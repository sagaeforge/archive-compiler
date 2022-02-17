#ifndef __DATATYPE_NSTRING_H__
#define __DATATYPE_NSTRING_H__

#pragma pack(push, 1)

#include <DataType.h>

typedef struct
{
  Wcs_t m_Value;
  Length_t m_Length;
} nString_t, *nString_ptr;

typedef struct nStringAryNode
{
  nString_ptr m_Value;
  struct nStringAryNode* Next;
} nStringAryNode_t, *nStringAryNode_ptr;

typedef enum
{
  k_nStringAry_None = 0,
  k_nStringAry_Ary = 1,
  k_nStringAry_List = 2,
} nStringAryType_t,
  nStringAryType_ptr;

typedef struct
{
  Length_t m_Size;
  Length_t m_Length;
  nStringAryType_t m_AryType;
  nString_ptr* m_Arys;
  nStringAryNode_ptr m_Lists;
} nStringAry_t, *nStringAry_ptr;

#pragma pack(pop)

#endif // __NSTRING_H__