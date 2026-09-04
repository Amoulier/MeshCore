#include "PersistentWriteGuard.h"

__attribute__((noinline)) bool meshcorePersistentWritesAllowed() __attribute__((weak));
__attribute__((noinline)) bool meshcorePersistentWritesAllowed()
{
  return true;
}
