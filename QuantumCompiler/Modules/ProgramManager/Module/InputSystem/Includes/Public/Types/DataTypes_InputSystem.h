
#ifndef __PUBLIC_DATATYPES_INPUTSYSTEM__
#define __PUBLIC_DATATYPES_INPUTSYSTEM__

#include "DataTypes.h"
#include "Types/DataTypes_String.h"

typedef struct
{
  // clang-format off
  String* (*Charator) ();
  String* (*Word)     ();
  String* (*Line)     ();
  String* (*Bool)     ();
  String* (*Digit)    ();
  String* (*Decimal)  ();
  String* (*Format)   ();
} Input;

typedef struct
{
  void (*Charator) (String *Value);
  void (*Word)     (String *Value);
  void (*Line)     (String *Value);
  void (*Bool)     ();
  void (*Digit)    ();
  void (*Decimal)  ();
  void (*Format)   (const String *Format, ...);
} Output;

typedef struct
{

} ErrorOutput;

typedef struct
{

} InputSystemMethod;

#endif