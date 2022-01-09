
#include "StringAry.h"
#include "GarbageCollection.h"
#include "Private_String.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _StringAryNode StringAryNode;

static StringAryNode*
StringAry_NodeCreate()
{
  StringAryNode* node = MemoryCreate(sizeof(StringAryNode));
  if (node == NULL)
    return NULL;
  node->Next = NULL;
  node->Value = NULL;
  return node;
}

StringAry*
StringAryConstructor(int Cnt, ...)
{
  StringAry* Ary = MemoryCreate(sizeof(StringAry));
  if (Ary == NULL)
    // TODO Exception 처리
    return NULL;

  Ary->Length = 0;
  Ary->Values = NULL;
  if (Cnt == 0)
    return Ary;

  va_list ap;
  va_start(ap, Cnt);
  int i;
  for (i = 0; i < Cnt; i++) {
    String* temp = va_arg(ap, String*);
    StringAry_Push(Ary, temp);
  }
  va_end(ap);
  return Ary;
}

void
StringAryDestructor(StringAry** Self)
{
  StringAry* Ary = *Self;
  if (Ary->Length != 0) {
    StringAryNode** ptr =
      (StringAryNode**)MemoryCreate(sizeof(StringAryNode*) * Ary->Length);

    StringAryNode* node = Ary->Values;
    int i;
    for (i = 0; node != NULL; i++) {
      ptr[i] = node;
      node = node->Next;
    }
    for (i = 0; i < Ary->Length; i++)
      MemoryRemove((void**)&ptr[i]);
    MemoryRemove((void**)ptr);
  }

  (*Self)->Values = NULL;
  (*Self)->Length = 0;

  MemoryRemove((void**)Self);
  (*Self) = NULL;
}

String*
StringAry_Get(StringAry* Self, Index Index)
{
  if (Index >= Self->Length)
    // TODO Exception 처리
    return NULL;

  StringAryNode* node = Self->Values;
  int i;
  for (i = 0; i < Index; i++)
    node = node->Next;
  return node->Value;
}

void
StringAry_Insert(StringAry* Self, String* Value, Index Index)
{
  if (Index >= Self->Length - 1)
    StringAry_Push(Self, Value);

  StringAryNode* node = StringAry_NodeCreate();
  node->Value = Value;
  node->Next = NULL;
  Self->Length++;

  StringAryNode* insertNode = Self->Values;
  if (Index == 0) {
    Self->Values = node;
    node->Next = insertNode;
    return;
  }

  int i;
  for (i = 0; i < Index - 1; i++)
    insertNode = insertNode->Next;

  StringAryNode* backup = insertNode;
  insertNode->Next = node;
  node->Next = backup;
}

void
StringAry_Remove(StringAry* Self, Index Index)
{
  Index = Index >= Self->Length ? Self->Length : Index;

  StringAryNode* node = Self->Values;
  StringAryNode* backup = node;
  Self->Length--;
  if (Index == 0) {
    node = node->Next;
    Self->Values = node;
    MemoryRemove((void**)&backup);
    return;
  }

  int i;
  for (i = 0; i < Index - 1; i++) {
    backup = node;
    node = node->Next;
  }
  node->Next = backup->Next;
  MemoryRemove((void**)&backup);
}
void
StringAry_Push(StringAry* Self, String* Value)
{
  StringAryNode* node = StringAry_NodeCreate();
  node->Next = NULL;
  node->Value = Value;

  if (Self->Values == NULL) {
    Self->Values = node;
    Self->Length++;
    return;
  }

  StringAryNode* InsertNode = Self->Values;
  while (InsertNode->Next != NULL)
    InsertNode = InsertNode->Next;

  InsertNode->Next = node;
  Self->Length++;
}
String*
StringAry_Pop(StringAry* Self)
{
  StringAryNode* node = Self->Values;
  StringAryNode* backup = node;
  if (Self->Length == 0)
    // TODO Exception 처리
    return NULL;

  String* temp = NULL;
  if (Self->Length == 1) {
    temp = Self->Values->Value;
    MemoryRemove((void**)&Self->Values);
    return temp;
  }

  while (node->Next != NULL) {
    backup = node;
    node = node->Next;
  }
  temp = node->Value;
  MemoryRemove((void**)&node);
  backup->Next = NULL;
  Self->Length--;
  return temp;
}

Index
StringAry_Search(StringAry* Self, String* Value)
{
  StringAryNode* node = Self->Values;
  int i;
  for (i = 0; node != NULL; i++)
    if (String_Compare(node->Value, Value))
      return i;
    else
      node = node->Next;
  return -1;
}

Length
StringAry_Contains(StringAry* Self, String* Value)
{
  return StringAry_Search(Self, Value) != -1;
}