#ifndef __NDIGITARY_H__
#define __NDIGITARY_H__

#include <Module/Types/nDigitAry.h>

// clang-format off

nDigitAry_ptr DigitAry_Constructor  ();
void          DigitAry_Destructor   (nDigitAry_ptr* pSelf);
DEFUALT_DIGIT DigitAry_get          (nDigitAry_ptr  pSelf, Index_t pIndex);
void          DigitAry_set          (nDigitAry_ptr  pSelf, Index_t pIndex, DEFUALT_DIGIT pValue);
void          DigitAry_Insert       (nDigitAry_ptr  pSelf, Index_t pIndex, DEFUALT_DIGIT pValue);
void          DigitAry_Remove       (nDigitAry_ptr  pSelf, Index_t pIndex);
void          DigitAry_Push         (nDigitAry_ptr  pSelf, DEFUALT_DIGIT pValue);
DEFUALT_DIGIT DigitAry_Pop          (nDigitAry_ptr  pSelf);
void          DigitAry_Join         (nDigitAry_ptr  pSelf, nDigitAry_ptr pTarget);
nDigitAry_ptr DigitAry_Split        (nDigitAry_ptr  pSelf, Index_t pIndex);

nDigitAryNode_ptr DigitAry_MakeNode ();

#endif // __NDIGITARY_H__