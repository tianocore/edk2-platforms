#include <PiPei.h>

UINT8
EFIAPI
FspGetModeSelection (
  VOID
  )
{
  return PcdGet8 (PcdFspModeSelection);
}