/***
 - GitHub AI_StackChan_Minimal
 https://github.com/A-Uta/AI_StackChan_Minimal

 - 利用Webサービスと料金
 1. OpenAI API (Pricing) - ChatGPT/Whisper
 https://openai.com/api/pricing/

 2. WEB版VOICEVOX API（無料）
 https://voicevox.su-shiki.com/su-shikiapis/

 3. Google Speech-to-Text の料金
 https://cloud.google.com/speech-to-text/pricing?hl=ja
***/

#include <Arduino.h>
#include <M5UnitGLASS2.h> // Add for SSD1306
#include <SPIFFS.h>       // Add for Web Setting
#include <M5Unified.h>
#include <nvs.h>          // Add for Web Setting
#include <Avatar.h>       // Add for M5 avatar
#include <AudioOutput.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputM5Speaker.h"
#include "AudioFileSourceHTTPSStream.h"
#include "WebVoiceVoxTTS.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"
#include <ArduinoJson.h>
#include "AudioWhisper.h"
#include "Whisper.h"
#include "Audio.h"
#include "CloudSpeechClient.h"
#include <deque>
#include <FastLED.h>
#include <ESP32WebServer.h> // Add for Web Setting

using namespace m5avatar;   // Add for M5 avatar
Avatar avatar;              // Add for M5 avatar
ESP32WebServer server(80);  // Add for Web Setting

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;

/// 接続：Wifiは[スマホアプリ(EspTouch)]、また[APIキーはブラウザ]から設定します。

/// I2C接続のピン番号 // Add for SSD1306
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 25
/// LEDストリップのピン番号
#define LED_PIN     27
/// LEDストリップのLED数
#define NUM_LEDS    1
/// 明るさ
#define BRIGHTNESS 64 // Adjust for Atom Echo
/// LEDストリップの色の並び順
#define COLOR_ORDER GRB
/// FastLEDライブラリの初期化
CRGB leds[NUM_LEDS];
/// 保存する質問と回答の最大数
const int MAX_HISTORY = 2; // Adjust for Atom Echo
/// 過去の質問と回答を保存するデータ構造
std::deque<String> chatHistory;
///---------------------------------------------
String OPENAI_API_KEY = "";
String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
String TTS_SPEAKER_NO = "3";  // [ずんだもん] 1:あまあま 3:ノーマル, 38:ヒソヒソ <See https://puarts.com/?pid=1830 >
String TTS_SPEAKER = "&speaker=";
String TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
String speech_text_buffer = "";

// DynamicJsonDocument chat_doc(1024*10);
StaticJsonDocument<768> chat_doc; // Adjust for Atom Echo
String json_ChatString = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
// String json_ChatString = "{\"model\": \"gpt-4o\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
// String json_ChatString =
// "{\"model\": \"gpt-3.5-turbo\",\
//  \"messages\": [\
//                 {\"role\": \"user\", \"content\": \"" + text + "\"},\
//                 {\"role\": \"system\", \"content\": \"あなたは「スタックちゃん」と言う名前の小型ロボットとして振る舞ってください。\"},\
//                 {\"role\": \"system\", \"content\": \"あなたはの使命は人々の心を癒すことです。\"},\
//                 {\"role\": \"system\", \"content\": \"幼い子供の口調で話してください。\"},\
//                 {\"role\": \"system\", \"content\": \"語尾には「だよ｝をつけて話してください。\"}\
//               ]}";
String Role_JSON = "";
String InitBuffer = "";

static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>AIｽﾀｯｸﾁｬﾝ</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>AIｽﾀｯｸﾁｬﾝ - APIキー設定</title>
  </head>
  <body>
    <h1>APIキー設定</h1>
    <form>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)">
      <label for="role1">OpenAI(必須)</label></br>
      <input type="text" id="voicevox" name="voicevox" oninput="adjustSize(this)">
      <label for="role2">VoiceVox(必須)</label></br>
      <input type="text" id="sttapikey" name="sttapikey" oninput="adjustSize(this)">
      <label for="role3">Google Speech to Text(未入力時,OpenAI Whisperを使用)</label></br>
      <button type="button" onclick="sendData()">送信する</button>
    </form>
    <script>
      function adjustSize(input) {
        input.style.width = ((input.value.length + 1) * 8) + 'px';
      }
      function sendData() {
        // FormDataオブジェクトを作成
        const formData = new FormData();

        // 各ロールの値をFormDataオブジェクトに追加
        const openaiValue = document.getElementById("openai").value;
        if (openaiValue !== "") formData.append("openai", openaiValue);

        const voicevoxValue = document.getElementById("voicevox").value;
        if (voicevoxValue !== "") formData.append("voicevox", voicevoxValue);

        const sttapikeyValue = document.getElementById("sttapikey").value;
        if (sttapikeyValue !== "") {
          formData.append("sttapikey", sttapikeyValue);
        } else {
          formData.append("sttapikey", openaiValue);
        }

	    // POSTリクエストを送信
	    const xhr = new XMLHttpRequest();
	    xhr.open("POST", "/apikey_set");
	    xhr.onload = function() {
	      if (xhr.status === 200) {
	        alert("データを送信しました！");
	      } else {
	        alert("送信に失敗しました。");
	      }
	    };
	    xhr.send(formData);
	  }
	</script>
  </body>
</html>)KEWL";

/// LED Color
// CRGB::Black, Red, Green, Blue, Pink, Yellow, MidnightBlue, LightBlue, Orange, Magenta, MediumBlue, LightGreen
void set_led_color(CRGB col){
  leds[0] = col;
  FastLED.show();
}

void handleRoot() {
  String message = "";
  message += "<h1>設定メニュー</h1>";
  message += "\n<ul>";
  message += "\n  <li><a href='apikey'>APIキー設定</a></li>";
  message += "\n  <li>ChatGPTモデルの変更</li>";
  message += "\n  <li>キャラクター音声の変更</li>";
  message += "\n  <li>テキストで会話</li>";
  message += "\n  <li>ロール設定</li>";
  message += "\n</ul>";
  server.send(200, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handle_apikey() {
  /// ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", APIKEY_HTML);
}

void handle_apikey_set() {
  /// POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  /// openai
  String openai = server.arg("openai");
  /// voicetxt
  String voicevox = server.arg("voicevox");
  /// voicetxt
  String sttapikey = server.arg("sttapikey");
 
  OPENAI_API_KEY = openai;
  VOICEVOX_API_KEY = voicevox;
  STT_API_KEY = sttapikey;
  Serial.println(openai);
  Serial.println(voicevox);
  Serial.println(sttapikey);

  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
    nvs_set_str(nvs_handle, "openai", openai.c_str());
    nvs_set_str(nvs_handle, "voicevox", voicevox.c_str());
    nvs_set_str(nvs_handle, "sttapikey", sttapikey.c_str());
    nvs_close(nvs_handle);
  }
  set_led_color(CRGB::Green);
  avatar.setExpression(Expression::Happy);
  avatar.setSpeechText("APIキーが、セットされました");
  server.send(200, "text/plain", String("OK"));
  delay(3000);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
  set_led_color(CRGB::Black);
}

bool init_chat_doc(const char *data)
{
  DeserializationError error = deserializeJson(chat_doc, data);
  if (error) {
    Serial.println("DeserializationError");
    return false;
  }
  String json_str; //= JSON.stringify(chat_doc);
  serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
//  Serial.println(json_str);
    return true;
}

String https_post_json(const char* url, const char* json_string, const char* root_ca) {
  String payload = "";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      https.setTimeout( 65000 ); 
  
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url)) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
        int httpCode = https.POST((uint8_t *)json_string, strlen(json_string));
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            Serial.println("//////////////");
            Serial.println(payload);
            Serial.println("//////////////");
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }
  return payload;
}

String chatGpt(String json_string) {
  String response = "";
  Serial.print("chatGpt = ");
  Serial.println(json_string);
  avatar.setExpression(Expression::Doubt);
  avatar.setSpeechText("考え中…");
  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
  Serial.println(ret);
  if(ret != ""){
    // DynamicJsonDocument doc(2000);
    StaticJsonDocument<2000> doc;  // Adjust for Atom Echo
    DeserializationError error = deserializeJson(doc, ret.c_str());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("エラーです");
      response = "エラーです";
      // delay(1000);
      delay(700); // Adjust for Atom Echo
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    }else{
      const char* data = doc["choices"][0]["message"]["content"];
      Serial.println(data);
      response = String(data);
      std::replace(response.begin(),response.end(),'\n',' ');
    }
  } else {
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("わかりません");
    response = "わかりません";
    // delay(1000);
    delay(700); // Adjust for Atom Echo
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  }
  return response;
}

String exec_chatGPT(String text) {
  static String response = "";
  Serial.println(InitBuffer);
  init_chat_doc(InitBuffer.c_str());
  /// 質問をチャット履歴に追加
  chatHistory.push_back(text);
  /// チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    if(i % 2 == 0) {
      systemMessage1["role"] = "user";
    } else {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }

  String json_string;
  serializeJson(chat_doc, json_string);
  response = chatGpt(json_string);
  /// 返答をチャット履歴に追加
  chatHistory.push_back(response);
  // Serial.printf("chatHistory.max_size %d \n",chatHistory.max_size());
  // Serial.printf("chatHistory.size %d \n",chatHistory.size());
  // for (int i = 0; i < chatHistory.size(); i++)
  // {
  //   Serial.print(i);
  //   Serial.println("= "+chatHistory[i]);
  // }
  serializeJsonPretty(chat_doc, json_string);
  Serial.println("====================");
  Serial.println(json_string);
  Serial.println("====================");

  return response;
}

void playMP3(AudioFileSourceBuffer *buff){
  mp3->begin(buff, &out);
}

String SpeechToText(bool isGoogle){
  Serial.println("\r\nRecord start!\r\n");

  String ret = "";
  if( isGoogle) {
    Audio* audio = new Audio();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    // avatar.setSpeechText("わかりました");  
    set_led_color(CRGB::Orange);
    CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(root_ca_google, STT_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  } else {
    AudioWhisper* audio = new AudioWhisper();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    // avatar.setSpeechText("わかりました");  
    set_led_color(CRGB::Orange);
    Whisper* cloudSpeechClient = new Whisper(root_ca_openai, OPENAI_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  }
  return ret;
}


/// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  /// Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  /// Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void lipSync(void *args)  // Add for M5 avatar
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    level = abs(*out.getBuffer());
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    avatar->setMouthOpenRatio(open);
    // avatar->getGaze(&gazeY, &gazeX);
    // avatar->setRotation(gazeX * 5);
    delay(50);
  }
}

void Wifi_setup() { // Add for Web Setting (SmartConfig)
  // Serial.println("WiFiに接続中");
  Serial.println("接続中:WiFi"); M5.Display.println("接続中:WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA); 
  WiFi.begin();

  /// 前回接続時情報で接続する
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); M5.Display.print(".");
    delay(500);
    /// 10秒以上接続できなかったら抜ける
    if ( 10000 < millis() ) {
      break;
    }
  }
  Serial.println(""); M5.Display.println("");
  /// 未接続の場合にはSmartConfig待受
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    Serial.println("接続中:SmartConfig"); M5.Display.println("接続中:SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print("#"); M5.Display.print("#");
      /// 30秒以上接続できなかったら抜ける
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    /// Wi-fi接続
    Serial.println(""); M5.Display.println("");
    Serial.println("接続中:WiFi"); M5.Display.println("接続中:WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("."); M5.Display.print(".");
      /// 60秒以上接続できなかったら抜ける
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
  }
}


void setup()
{
  auto cfg = M5.config();
	cfg.unit_glass2.pin_sda= I2C_SDA_PIN; // Add for SSD1306
	cfg.unit_glass2.pin_scl= I2C_SCL_PIN; // Add for SSD1306

  M5.begin(cfg);
	M5.setPrimaryDisplayType({m5::board_t::board_M5UnitGLASS2}); // Add for M5 avatar
  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }
  M5.Speaker.begin();
  /// set master volume (0~255)
  M5.Speaker.setVolume(150);  // Adjust for Atom Echo (DO NOT set over 150. This echo Speaker will be broken.)
  M5.Lcd.setFont(&fonts::lgfxJapanGothic_12); // Adjust for SSD1306 (Connect info)
  M5.Lcd.setTextSize(1);                      // Adjust for SSD1306 (Connect info)

  FastLED.addLeds<SK6812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.show();
  set_led_color(CRGB::Green);

  mp3 = new AudioGeneratorMP3();

  /// ESP32本体フラッシュメモリより APIキーを読み取り
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READONLY, &nvs_handle)) {
      Serial.println("nvs_open");

      size_t length1;
      size_t length2;
      size_t length3;
      if(ESP_OK == nvs_get_str(nvs_handle, "openai", nullptr, &length1) && 
         ESP_OK == nvs_get_str(nvs_handle, "voicevox", nullptr, &length2) && 
         ESP_OK == nvs_get_str(nvs_handle, "sttapikey", nullptr, &length3) && 
        length1 && length2 && length3) {
        Serial.println("nvs_get_str");
        char openai_apikey[length1 + 1];
        char voicevox_apikey[length2 + 1];
        char stt_apikey[length3 + 1];
        if(ESP_OK == nvs_get_str(nvs_handle, "openai", openai_apikey, &length1) && 
           ESP_OK == nvs_get_str(nvs_handle, "voicevox", voicevox_apikey, &length2) &&
           ESP_OK == nvs_get_str(nvs_handle, "sttapikey", stt_apikey, &length3)) {
          OPENAI_API_KEY = String(openai_apikey);
          VOICEVOX_API_KEY = String(voicevox_apikey);
          STT_API_KEY = String(stt_apikey);
        }
      }
      nvs_close(nvs_handle);
    }
  }

  /// ネットワークに接続
  Wifi_setup(); // Add for Web Setting
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("接続OK-WiFi");

  server.on("/", handleRoot);
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  /// And as regular external functions:
  server.on("/apikey", handle_apikey);
  server.on("/apikey_set", HTTP_POST, handle_apikey_set);
  server.onNotFound(handleNotFound);

  init_chat_doc(json_ChatString.c_str());
  /// SPIFFSをマウントする
  if(SPIFFS.begin(true)){
    /// JSONファイルを開く
    File file = SPIFFS.open("/data.json", "r");
    if(file){
      DeserializationError error = deserializeJson(chat_doc, file);
      if(error){
        Serial.println("Failed to deserialize JSON");
        init_chat_doc(json_ChatString.c_str());
      }
      serializeJson(chat_doc, InitBuffer);
      Role_JSON = InitBuffer;
      String json_str; 
      serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
      Serial.println(json_str);
    } else {
      Serial.println("Failed to open file for reading");
      init_chat_doc(json_ChatString.c_str());
    }
  } else {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }  

  server.begin(); // Add for Web Setting
  Serial.println("設定URL："); M5.Lcd.println("設定URL："); 
  M5.Lcd.setTextSize(1.3);              // Adjust for SSD1306 (Connect info)
  Serial.print(WiFi.localIP()); M5.Lcd.print(WiFi.localIP());
  delay(6000);

  avatar.setScale(.32);               // Adjust for SSD1306
  avatar.setPosition(-92,-100);       // Adjust for SSD1306
  avatar.init();                      // Add for M5 avatar
  avatar.addTask(lipSync, "lipSync");   // Add for M5 avatar
  avatar.setSpeechFont(&fonts::lgfxJapanGothic_16);  // Adjust for SSD1306

  set_led_color(CRGB::Black);
  M5.Lcd.setTextSize(1);                // Adjust for SSD1306 (Connect info)

}


void loop()
{
  M5.update();

  /// 設定URL表示
  if (M5.BtnA.wasDoubleClicked())
  {
      set_led_color(CRGB::Blue);
      M5.Speaker.tone(1000, 100);
      delay(100);
      M5.Speaker.end();
      delay(100);
      M5.Speaker.begin();
      String response2 = ""; 
      response2 = "設定URL：" + WiFi.localIP().toString();
      avatar.setExpression(Expression::Happy);
      avatar.setSpeechText(response2.c_str());
      delay(5000);
      set_led_color(CRGB::Black);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
  }

  /// おしゃべり開始
  if (M5.BtnA.wasSingleClicked())
  {
    M5.Speaker.tone(1000, 100);
    delay(200);
    avatar.setExpression(Expression::Happy);
    avatar.setSpeechText("御用でしょうか？");
    set_led_color(CRGB::Magenta);
    M5.Speaker.end();
    String ret;
    
    Serial.print("OPENAI_API_KEY: ");    Serial.println(OPENAI_API_KEY);
    if(OPENAI_API_KEY == ""){  // Add for Web Setting
      Serial.println("Error: API-Keyが未設定");
      ret = "キー未設定";
    } else if(OPENAI_API_KEY != STT_API_KEY){
      Serial.println("Google STT");
      ret = SpeechToText(true);
    } else {
      Serial.println("Whisper STT");
      ret = SpeechToText(false);
    }

    String You_said;
    You_said = "認識:" +  ret;
    avatar.setSpeechText(You_said.c_str()); // 認識した文字を表示
    delay(2500);

    Serial.println("音声認識終了");
    Serial.println("音声認識結果");
    M5.Speaker.begin();
    if(ret != "") {
      set_led_color(CRGB::LightGreen);
      Serial.println(ret);
      if (!mp3->isRunning()) {
        String response = ""; // Add for Web Setting
        if(OPENAI_API_KEY == ""){
          response = "初めに、URL：" + WiFi.localIP().toString() + "をブラウザに入力し、APIキーを設定してください";
        } else {
          response = exec_chatGPT(ret);
        }

        if(response == "エラーです") {
          set_led_color(CRGB::Red);
          avatar.setSpeechText("エラーです"); // Add for M5 avatar
          avatar.setExpression(Expression::Sad);
          TTS_PARMS = TTS_SPEAKER + "38"; // 38:ヒソヒソ
          Voicevox_tts((char*)response.c_str(), (char*)TTS_PARMS.c_str());             
        } else if (response != "") {
          set_led_color(CRGB::Blue);
          avatar.setSpeechText(response.c_str()); // Add for M5 avatar
          avatar.setExpression(Expression::Happy);
          Voicevox_tts((char*)response.c_str(), (char*)TTS_PARMS.c_str());             
        }
      }
    } else {
      Serial.println("音声認識失敗");
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("聞き取れませんでした");
      String response = "聞き取れませんでした";
      TTS_PARMS = TTS_SPEAKER + "38"; // 38:ヒソヒソ
      Voicevox_tts((char*)response.c_str(), (char*)TTS_PARMS.c_str());   // add for M5 avatar
      set_led_color(CRGB::Red);
      delay(2000);
      set_led_color(CRGB::Black);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    } 
    /// Set Normal speaker
    TTS_PARMS = TTS_SPEAKER + "3"; // 3:ノーマル
  }

  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      if(file != nullptr){delete file; file = nullptr;}
      Serial.println("mp3 stop");
      avatar.setExpression(Expression::Neutral);
      speech_text_buffer = "";
      set_led_color(CRGB::Black);
      avatar.setSpeechText("");
      delay(5);
    }
  } else {
    server.handleClient();  // Add for Web Setting
  }
}
