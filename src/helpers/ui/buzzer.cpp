#include "Arduino.h"
#ifdef PIN_BUZZER
#include "buzzer.h"
#ifdef MUZI_BASE_SUPERIO
static genericBuzzer* playback_owner = nullptr;
#endif

void genericBuzzer::begin() {
//    Serial.print("DBG: Setting up buzzer on pin ");
//    Serial.println(PIN_BUZZER);
    #ifdef PIN_BUZZER_EN
      pinMode(PIN_BUZZER_EN, OUTPUT);
      digitalWrite(PIN_BUZZER_EN, HIGH);
    #endif

    quiet(false);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW); // need to pull low by default to avoid extreme power draw
}

void genericBuzzer::play(const char *melody) {
#ifdef MUZI_BASE_SUPERIO
    if (_is_quiet || melody == nullptr) return;
    if (playback_owner && playback_owner != this && rtttl::isPlaying()) return;
    if (isPlaying()) rtttl::stop();
    // RTTTL retains the pointer throughout playback.
    strncpy(_melody, melody, sizeof(_melody) - 1);
    _melody[sizeof(_melody) - 1] = 0;
    rtttl::begin(PIN_BUZZER, _melody);
    _owns_playback = true;
    playback_owner = this;
#else
    if (isPlaying())   // interrupt existing
    {
        rtttl::stop();
    }

    if (_is_quiet) return;

    rtttl::begin(PIN_BUZZER,melody);
#endif
//    Serial.print("DBG: Playing melody - isQuiet: ");
//    Serial.println(isQuiet());
}

bool genericBuzzer::isPlaying() {
#ifdef MUZI_BASE_SUPERIO
    return playback_owner == this && _owns_playback && rtttl::isPlaying();
#else
    return rtttl::isPlaying();
#endif
}

void genericBuzzer::loop() {
#ifdef MUZI_BASE_SUPERIO
    if (playback_owner != this || !_owns_playback) return;
#endif
    if (!rtttl::done()) rtttl::play();
#ifdef MUZI_BASE_SUPERIO
    if (rtttl::done()) { _owns_playback = false; playback_owner = nullptr; }
#endif
}

void genericBuzzer::startup() {
    play(startup_song);
}

void genericBuzzer::shutdown() {
    play(shutdown_song);
}

void genericBuzzer::quiet(bool buzzer_state) {
    _is_quiet = buzzer_state;
#ifdef MUZI_BASE_SUPERIO
    if (_is_quiet && _owns_playback && playback_owner == this) {
      rtttl::stop();
      _owns_playback = false;
      playback_owner = nullptr;
      digitalWrite(PIN_BUZZER, LOW);
    }
#endif
#ifdef PIN_BUZZER_EN
    if (_is_quiet) {
      digitalWrite(PIN_BUZZER_EN, LOW);
    } else {
      digitalWrite(PIN_BUZZER_EN, HIGH);
    }
#endif
}

bool genericBuzzer::isQuiet() {
    return _is_quiet;
}

#endif  // ifdef PIN_BUZZER
