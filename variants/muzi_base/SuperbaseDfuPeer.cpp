#if defined(MUZI_BASE_SUPERIO) && defined(NRF52_PLATFORM)
#include "SuperbaseDFU.h"
#include "SuperbaseDfuPeer.h"

namespace {
const volatile uint8_t* peerVerifier() {
  if (NRF_UICR->NRFFW[0] != 0xF4000) return nullptr;
  constexpr uintptr_t addresses[] = {0xF4610, 0xF4614};
  for (uintptr_t address : addresses) {
    const auto* verifier = reinterpret_cast<const volatile uint8_t*>(address);
    if (superbase::muziPeerCrcFirst(verifier)) return verifier;
  }
  return nullptr;
}
}

bool superbaseDfuPeerCrcFirst() {
  return peerVerifier() != nullptr;
}

// Called immediately after Bluefruit calculates the peer CRC and before its
// original disconnect/SoftDevice shutdown/jump. No keys are exported or saved.
void superbaseDfuAdaptPeerHandoff() {
  const auto* verifier = peerVerifier();
  if (verifier) superbase::adaptDfuPeer(reinterpret_cast<volatile uint8_t*>(0x20007F80), verifier);
}
#endif
