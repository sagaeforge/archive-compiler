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

typedef struct
{
  Length_t m_Length;
  bool m_isAry;
  union
  {
    nString_ptr* m_Values;
    nStringAryNode_ptr m_Nodes;
  };
} nStringAry_t, *nStringAry_ptr;

#pragma pack(pop)

#endif // __NSTRING_H__