
#include <Chs.h>

bool
__IsSpace(int ch)
{
  return (ch >= 9 && ch <= 13) || ch == 32;
}