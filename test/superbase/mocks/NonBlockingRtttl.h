#pragma once
namespace rtttl {
inline bool playing = false;
inline int starts = 0, stops = 0, advances = 0;
inline const char* melody = nullptr;
inline void begin(int, const char* text) { playing = true; melody = text; starts++; }
inline void stop() { playing = false; stops++; }
inline bool isPlaying() { return playing; }
inline bool done() { return !playing; }
inline void play() { advances++; }
}
