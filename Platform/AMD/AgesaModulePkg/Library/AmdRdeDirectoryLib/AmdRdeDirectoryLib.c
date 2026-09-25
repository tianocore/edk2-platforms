// Null stub implementations for standalone AmdPlatformPkg build
#include <Uefi.h>
#include <AmdRdeDirectory.h>

EFI_STATUS EFIAPI AmdRdeDirectoryGetImage (
  IN      UINT16  ImageType,
  IN      UINT64  *InMediaOffset OPTIONAL,
  IN OUT  UINT32  *Imagesize,
  OUT     VOID    *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
