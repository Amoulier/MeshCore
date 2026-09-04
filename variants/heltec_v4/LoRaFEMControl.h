#pragma once

#include <stdint.h>

#ifndef DEFAULT_FEM_RX_GAIN
#define DEFAULT_FEM_RX_GAIN 0
#endif

typedef enum {
  GC1109_PA,
  KCT8103L_PA,
  OTHER_FEM_TYPES
} LoRaFEMType;

class LoRaFEMControl {
public:
  LoRaFEMControl() {}
  virtual ~LoRaFEMControl() {}

  void init();
  void setSleepModeEnable();
  void setTxModeEnable();
  void setRxModeEnable();
  void setRxModeEnableWhenMCUSleep();
  void setLNAEnable(bool enabled);
  bool isLnaCanControl() const { return lna_can_control; }
  void setLnaCanControl(bool can_control) { lna_can_control = can_control; }
  bool isLNAEnabled() const { return lna_enabled; }
  LoRaFEMType getFEMType() const { return fem_type; }

private:
  LoRaFEMType fem_type = OTHER_FEM_TYPES;
  bool lna_enabled = DEFAULT_FEM_RX_GAIN != 0;
  bool lna_can_control = false;
};
