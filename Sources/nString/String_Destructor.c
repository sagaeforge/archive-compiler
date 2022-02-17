
#include <stdlib.h>

#include <Module/nString.h>

bool
String_Destructor(nString_t** pSelf)
{
  if (!pSelf || !(*pSelf))
    return false;

  (*pSelf)->m_Length = 0;
  free((*pSelf)->m_Value);
  (*pSelf)->m_Value = NULL;
  free((*pSelf));
  (*pSelf) = NULL;
  return true;
}
