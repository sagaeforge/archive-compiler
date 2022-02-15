#ifndef __DATATYPE_H__
#define __DATATYPE_H__

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

typedef char* Chs_t;
typedef wchar_t* Wcs_t;

typedef uint32_t Length_t;
typedef uint32_t Index_t;

#define LOOPEND(Instance) Instance&(1 << 31)
#define BitAndMask(n) ((1 << n) - 1)

typedef void (*Func_t)();

#endif // __DATATYPE_H__