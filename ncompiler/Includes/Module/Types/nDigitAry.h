#ifndef __DATATYPE_NDIGITARY_H__
#define __DATATYPE_NDIGITARY_H__

#include <Module/Types/DataType.h>

#define DEFUALT_DIGIT int64_t

typedef struct nDigitAryNode
{
  DEFUALT_DIGIT m_Value;
  struct nDigitAryNode* Next;
} nDigitAryNode_t, *nDigitAryNode_ptr;

typedef struct nDigitAry
{
  Length_t m_Length;
  nDigitAryNode_ptr m_Nodes;
} nDigitAry_t, *nDigitAry_ptr;

#endif // __NDIGITARY_H__