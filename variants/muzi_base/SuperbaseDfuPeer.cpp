#if defined(MUZI_BASE_SUPERIO) && defined(NRF52_PLATFORM)
#include "SuperbaseDFU.h"
#include "SuperbaseDfuPeer.h"

bool superbaseDfuPeerCrcFirst() {
  return NRF_UICR->NRFFW[0] == 0xF4000 &&
    superbase::muziPeerCrcFirst(reinterpret_cast<const volatile uint8_t*>(0xF4610));
}

// Called immediately after Bluefruit calculates the peer CRC and before its
// original disconnect/SoftDevice shutdown/jump. No keys are exported or saved.
void superbaseDfuAdaptPeerHandoff() {
  if (NRF_UICR->NRFFW[0] != 0xF4000) return;
  superbase::adaptDfuPeer(reinterpret_cast<volatile uint8_t*>(0x20007F80),
    reinterpret_cast<const volatile uint8_t*>(0xF4610));
}
#endif
