#define RADIOLIB_STATIC_ONLY 1
#include "RadioLibWrappers.h"

#define STATE_IDLE 0
#define STATE_RX 1
#define STATE_TX_WAIT 3
#define STATE_TX_DONE 4
#define STATE_INT_READY 16

#define NUM_NOISE_FLOOR_SAMPLES 64
#define SAMPLING_THRESHOLD 14

static volatile uint8_t state = STATE_IDLE;

static
#if defined(ESP8266) || defined(ESP32)
    ICACHE_RAM_ATTR
#endif
    void
    setFlag()
{
  state |= STATE_INT_READY;
}

void RadioLibWrapper::begin()
{
  _radio->setPacketReceivedAction(setFlag);
  _preamble_sf = getSpreadingFactor();
  _radio->setPreambleLength(preambleLengthForSF(_preamble_sf));
  state = STATE_IDLE;

  if (_board->getStartupReason() == BD_STARTUP_RX_PACKET) {
    setFlag();
  }

  _noise_floor = 0;
  _threshold = 0;
  _cad_enabled = false;
  _num_floor_samples = 0;
  _floor_sample_sum = 0;
}

uint32_t RadioLibWrapper::getRngSeed()
{
  return _radio->random(0x7FFFFFFF);
}

void RadioLibWrapper::setTxPower(int8_t dbm)
{
#if defined(USE_LR2021)
  idle();
#endif
  _radio->setOutputPower(_board->mapRadioTxPower(dbm));
}

void RadioLibWrapper::idle()
{
  _radio->standby();
  state = STATE_IDLE;
}

void RadioLibWrapper::triggerNoiseFloorCalibrate(int threshold)
{
  _threshold = threshold;
  if (_num_floor_samples >= NUM_NOISE_FLOOR_SAMPLES) {
    _num_floor_samples = 0;
    _floor_sample_sum = 0;
  }
}

void RadioLibWrapper::doResetAGC()
{
  _radio->sleep();
}

void RadioLibWrapper::resetAGC()
{
  if ((state & STATE_INT_READY) != 0 || isReceivingPacket()) {
    return;
  }

  doResetAGC();
  state = STATE_IDLE;
  _noise_floor = 0;
  _num_floor_samples = 0;
  _floor_sample_sum = 0;
}

void RadioLibWrapper::loop()
{
  _board->loop();

  if (state == STATE_RX && _num_floor_samples < NUM_NOISE_FLOOR_SAMPLES) {
    if (!isReceivingPacket()) {
      const int rssi = getCurrentRSSI();
      if (rssi < _noise_floor + SAMPLING_THRESHOLD) {
        _num_floor_samples++;
        _floor_sample_sum += rssi;
      }
    }
  } else if (_num_floor_samples >= NUM_NOISE_FLOOR_SAMPLES && _floor_sample_sum != 0) {
    _noise_floor = _floor_sample_sum / NUM_NOISE_FLOOR_SAMPLES;
    if (_noise_floor < -120) {
      _noise_floor = -120;
    }
    _floor_sample_sum = 0;

#ifdef MESH_DEBUG_NOISE_FLOOR
    MESH_DEBUG_PRINTLN("RadioLibWrapper: noise_floor = %d", static_cast<int>(_noise_floor));
#endif
  }
}

void RadioLibWrapper::startRecv()
{
#if defined(USE_LR2021)
  _radio->standby();
#endif
  const int err = _radio->startReceive();
  if (err == RADIOLIB_ERR_NONE) {
    state = STATE_RX;
  } else {
    MESH_DEBUG_PRINTLN("RadioLibWrapper: error: startReceive(%d)", err);
  }
}

bool RadioLibWrapper::isInRecvMode() const
{
  return (state & ~STATE_INT_READY) == STATE_RX;
}

int RadioLibWrapper::recvRaw(uint8_t *bytes, int sz)
{
  int len = 0;
  if (state & STATE_INT_READY) {
    len = _radio->getPacketLength();
    if (len > 0) {
      if (len > sz) {
        len = sz;
      }
      const int err = _radio->readData(bytes, len);
      if (err != RADIOLIB_ERR_NONE) {
        MESH_DEBUG_PRINTLN("RadioLibWrapper: error: readData(%d)", err);
        len = 0;
        n_recv_errors++;
      } else {
        n_recv++;
      }
    }
#if defined(USE_LR2021)
    state = STATE_RX;
#else
    state = STATE_IDLE;
#endif
  }

  if (state != STATE_RX) {
    const int err = _radio->startReceive();
    if (err == RADIOLIB_ERR_NONE) {
      state = STATE_RX;
    } else {
      MESH_DEBUG_PRINTLN("RadioLibWrapper: error: startReceive(%d)", err);
    }
  }
  return len;
}

uint32_t RadioLibWrapper::getEstAirtimeFor(int len_bytes)
{
  return _radio->getTimeOnAir(len_bytes) / 1000;
}

bool RadioLibWrapper::startSendRaw(const uint8_t *bytes, int len)
{
  _board->onBeforeTransmit();
  const int err = _radio->startTransmit(const_cast<uint8_t *>(bytes), len);
  if (err == RADIOLIB_ERR_NONE) {
    state = STATE_TX_WAIT;
    return true;
  }
  MESH_DEBUG_PRINTLN("RadioLibWrapper: error: startTransmit(%d)", err);
  idle();
  _board->onAfterTransmit();
  return false;
}

bool RadioLibWrapper::isSendComplete()
{
  if (state & STATE_INT_READY) {
    state = STATE_IDLE;
    n_sent++;
    return true;
  }
  return false;
}

void RadioLibWrapper::onSendFinished()
{
  _radio->finishTransmit();
  _board->onAfterTransmit();
  state = STATE_IDLE;
}

int16_t RadioLibWrapper::performChannelScan()
{
  return _radio->scanChannel();
}

bool RadioLibWrapper::isChannelActive()
{
  if (_threshold != 0 && getCurrentRSSI() > _noise_floor + _threshold) {
    return true;
  }

  if (_cad_enabled) {
    const int16_t result = performChannelScan();
    state = STATE_IDLE;
    startRecv();
    if (result != RADIOLIB_CHANNEL_FREE) {
      return true;
    }
  }

  return false;
}

float RadioLibWrapper::getLastRSSI() const
{
  return _radio->getRSSI();
}

float RadioLibWrapper::getLastSNR() const
{
  return _radio->getSNR();
}

static float snr_threshold[] = {
    -7.5,
    -10,
    -12.5,
    -15,
    -17.5,
    -20,
};

float RadioLibWrapper::packetScoreInt(float snr, int sf, int packet_len)
{
  if (sf < 7) {
    return 0.0f;
  }
  if (snr < snr_threshold[sf - 7]) {
    return 0.0f;
  }

  const float success_rate_based_on_snr = (snr - snr_threshold[sf - 7]) / 10.0f;
  const float collision_penalty = 1 - (packet_len / 256.0f);
  return max(0.0f, min(1.0f, success_rate_based_on_snr * collision_penalty));
}

PacketMillis RadioLibWrapper::calcMaxPacketMillis(uint8_t sf, float bw, uint8_t cr, uint8_t preambleSymbols)
{
  const uint32_t tsym_us = (static_cast<uint32_t>(10000) << sf) / (bw * 10);
  const uint32_t sfCoeff1_x4 = (sf == 5 || sf == 6) ? 25 : 17;
  const uint32_t preamble_us = (((preambleSymbols + 8) * 4 + sfCoeff1_x4) * tsym_us) / 4;

  const uint32_t total_us = _radio->getTimeOnAir(MAX_TRANS_UNIT);
  uint32_t payload_us = total_us > preamble_us ? total_us - preamble_us : 4000 - preamble_us;
  if (cr >= 5 && cr < 8) {
    payload_us = (payload_us * 8) / cr;
  }

  return PacketMillis{(preamble_us + 999) / 1000, (payload_us + 999) / 1000};
}
