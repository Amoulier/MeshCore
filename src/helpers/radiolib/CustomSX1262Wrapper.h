#pragma once

#include "CustomSX1262.h"
#include "RadioLibWrappers.h"
#include "SX126xReset.h"

#ifndef USE_SX1262
#define USE_SX1262
#endif

class CustomSX1262Wrapper : public RadioLibWrapper {
#ifdef MUZI_BASE_SUPERIO
  float _freq = LORA_FREQ, _bw = LORA_BW;
  uint8_t _sf = LORA_SF, _cr = 5;
  int8_t _tx = LORA_TX_POWER;
  bool _boosted = true;
protected:
  bool recoverReceive() override {
    auto* radio = static_cast<CustomSX1262*>(_radio);
    if (!radio->std_init()) return false;
    bool ok = radio->setFrequency(_freq) == RADIOLIB_ERR_NONE;
    ok = (radio->setCRC(1) == RADIOLIB_ERR_NONE) && ok;
    ok = (radio->setBandwidth(_bw) == RADIOLIB_ERR_NONE) && ok;
    ok = (radio->setSpreadingFactor(_sf) == RADIOLIB_ERR_NONE) && ok;
    ok = (radio->setCodingRate(_cr) == RADIOLIB_ERR_NONE) && ok;
    ok = (radio->setOutputPower(_tx) == RADIOLIB_ERR_NONE) && ok;
    ok = (radio->setRxBoostedGainMode(_boosted) == RADIOLIB_ERR_NONE) && ok;
    updatePreamble(_sf);
    return ok;
  }
#endif
public:
  CustomSX1262Wrapper(CustomSX1262& radio, mesh::MainBoard& board) : RadioLibWrapper(radio, board) { }

  void setParams(float freq, float bw, uint8_t sf, uint8_t cr) override {
#ifdef MUZI_BASE_SUPERIO
    _freq = freq; _bw = bw; _sf = sf; _cr = cr;
#endif
    ((CustomSX1262 *)_radio)->setFrequency(freq);
    ((CustomSX1262 *)_radio)->setSpreadingFactor(sf);
    ((CustomSX1262 *)_radio)->setBandwidth(bw);
    ((CustomSX1262 *)_radio)->setCodingRate(cr);
    updatePreamble(sf);
    PacketMillis pm = calcMaxPacketMillis(sf, bw, cr, preambleLengthForSF(sf));
    ((CustomSX1262 *)_radio)->setPreambleMillis(pm.preambleMillis);
    ((CustomSX1262 *)_radio)->setMaxPayloadMillis(pm.payloadMillis);
  }

  bool isReceivingPacket() override { 
    return ((CustomSX1262 *)_radio)->isReceiving();
  }
  float getCurrentRSSI() override {
    return ((CustomSX1262 *)_radio)->getRSSI(false);
  }
  float getLastRSSI() const override { return ((CustomSX1262 *)_radio)->getRSSI(); }
  float getLastSNR() const override { return ((CustomSX1262 *)_radio)->getSNR(); }

  float packetScore(float snr, int packet_len) override {
    int sf = ((CustomSX1262 *)_radio)->spreadingFactor;
    return packetScoreInt(snr, sf, packet_len);
  }
  uint8_t getSpreadingFactor() const override { return ((CustomSX1262 *)_radio)->spreadingFactor; }
  virtual void powerOff() override {
    ((CustomSX1262 *)_radio)->sleep(false);
  }

  bool setRxBoostedGainMode(bool en) override {
#ifdef MUZI_BASE_SUPERIO
    _boosted = en;
#endif
    return ((CustomSX1262 *)_radio)->setRxBoostedGainMode(en) == RADIOLIB_ERR_NONE;
  }
  bool getRxBoostedGainMode() const override {
    return ((CustomSX1262 *)_radio)->getRxBoostedGainMode();
  }
#ifdef MUZI_BASE_SUPERIO
  void setTxPower(int8_t dbm) override {
    _tx = dbm;
    RadioLibWrapper::setTxPower(dbm);
  }
#endif

  void doResetAGC() override { sx126xResetAGC((SX126x *)_radio, getRxBoostedGainMode()); }
};
