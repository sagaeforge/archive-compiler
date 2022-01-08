
#include "Private_StringLib.h"

String *String_ToString_Bool(bool Value) {
  return Value ? String("true") : String("false");
}