#ifndef _Whisper_H
#define _Whisper_H
#include <WiFiClientSecure.h>
#include "AudioWhisper.h"

class Whisper {
  WiFiClientSecure client;
  String key;
public:
  Whisper(const char* root_ca, const char* api_key);
  ~Whisper();
  // String Transcribe(AudioWhisper* audio);
  String Transcribe(AudioWhisper* audio, String LANG_CODE);  // Add for Global language
};

#endif // _Whisper_H

