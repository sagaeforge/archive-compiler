
#include <nString.h>

static bool String_valueOf_Bool(const nString_ptr pSelf) { return 0; }
static int64_t String_valueOf_Decimal(const nString_ptr pSelf) { return 0; }
static uint64_t String_valueOf_Decimal_Unsigned(const nString_ptr pSelf) {
  return 0;
}
static double String_valueOf_Digit(const nString_ptr pSelf) { return 0; }

Func_t String_ValueOfSearch(const char *DataType) { return NULL; }