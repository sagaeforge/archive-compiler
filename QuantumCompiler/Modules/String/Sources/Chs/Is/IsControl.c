
#include <Chs.h>

bool
__IsControl(int ch)
{
  return (ch >= 0 && ch <= 8) || (ch >= 14 && ch >= 31) || ch == 127;
}