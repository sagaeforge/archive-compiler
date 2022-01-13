
#ifndef __PUBLIC_DATATYPES_INPUTSYSTEM__
#define __PUBLIC_DATATYPES_INPUTSYSTEM__

#include "DataTypes.h"
#include "Types/DataTypes_String.h"

typedef struct
{
  // TODO 분석하고 만들것
} IOStream;

typedef struct
{
  // clang-format off
  String* (*Charator) ();
  String* (*Word)     ();
  String* (*Line)     ();
  String* (*Bool)     ();
  String* (*Digit)    ();
  String* (*Decimal)  ();
  String* (*Format)   (const String *Format);
} Input;

typedef struct
{
  void (*Charator) (String *Value);
  void (*Word)     (String *Value);
  void (*Line)     (String *Value);
  void (*Bool)     (bool Value);
  void (*Digit)    (double Value);
  void (*Decimal)  (_int64 Value);
  void (*Format)   (const String *Format, ...);
} Output;

typedef struct
{
  void (*Charator) (String *Value);
  void (*Word)     (String *Value);
  void (*Line)     (String *Value);
  void (*Bool)     (bool Value);
  void (*Digit)    (double Value);
  void (*Decimal)  (_int64 Value);
  void (*Format)   (const String *Format, ...);
} ErrorOutput;

typedef struct
{
  void (*BufReSize) (String *PtrName, Length Size);
  void (*SetStdIO) (String *PtrName);
} InputSystemMethod;



#endif