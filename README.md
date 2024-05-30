# AI_StackChan_Minimal
AIスタックチャン ミニマル(Minimal)です。β版として雑な公開ですが、改善点をいただければ幸いです。
<br><br>

<img src="images/AI_StackChan_Minimal01.jpg" width="50%"><br><br>

特徴<br>
- 知ってしまった人が勢いで始められるよう「おもちゃ感覚で使えるミニマル構成」にリ・デザイン[安く/作り易く(はんだ作業不要)/操作し易い]
- robo8080さんの"AIｽﾀｯｸﾁｬﾝ2"および"ATOM Echo版"をベースに、必要最低限の機能を搭載(サーボ動作,ウェイクワード等除く)
- アプリおよびWebブラウザより、Wifi/APIキー等が設定できます(プログラムへの直接記載、およびSDカードへの記載が不要)
- 使用Webサービス
	- 対話型生成AI：ChatGPT
		- https://platform.openai.com/docs/overview
	- 音声合成：Web版 VOICEVOX
		- https://voicevox.su-shiki.com/su-shikiapis/
	- 音声認識："OpenAI Whisper"か"Google Cloud STT"のどちらかを選択
		- https://cloud.google.com/speech-to-text?hl=ja
<br>

感謝<br>
- まずは命を削ってAIスタックチャンを公開してくれた、robo8080さんに大大感謝いたします。<br>
- Google Cloud STTは、”MhageGH”さんの [esp32_CloudSpeech](https://github.com/MhageGH/esp32_CloudSpeech/ "Title") を参考にさせて頂きました。ありがとうございました。<br>
- "OpenAI Whisper"が使えるようにするにあたって、多大なご助言を頂いた”イナバ”さん、”kobatan”さんに感謝致します。<br>
---


### AIスタックチャン ミニマルを作るのに必要な物、及び作り方 ###
Atom Echoだけでも動作します！が、以下のモノを組立てることで【より可愛いミニマル】に仕上がります<br>
-  ATOM Echo(マイコン本体)[約2,500円]
	- プログラムを動作させる本体。ディスプレイをつなぐとｽﾀｯｸﾁｬﾝの顔と文字でコミュニケーションができます。
		-  https://www.switch-science.com/products/6347
-  有機ELディスプレイ(0.96インチ 128×64ドット)[約600円]
	- SSD1306を搭載かつ、外装ケースに入る サイズ：幅：25.2mm x 高さ：26mmを想定しています。
		- https://akizukidenshi.com/catalog/g/g115870
- ジャンパーワイヤ[メス-オス）（10cm）[約700円]
	- ATOM Echoと有機ELディスプレイを接続するのに使用します。10cmでなくても構いませんが、外装に入る短いワイヤーをお使いください。
		- https://www.amazon.co.jp/dp/B072N2WR5N/
- 外装ケース[無料]
	- このページで3Dモデルを公開しています。3Dプリンタで印刷ください。
		-https://github.com/A-Uta/AI_StackChan_Minimal/tree/main/AI_StackChan_Minimal/3D_model 

### 制作の流れ ###
![画像1](images/making_all01_A.jpg)<br><br>

### プログラムをビルドするのに必要な物 ###
* [ATOM ECHO](https://docs.m5stack.com/ja/atom/atomecho/ "Title")<br>
* VSCode<br>
* PlatformIO<br>

使用しているライブラリ等は"platformio.ini"を参照してください。<br>

【2024/5/30の時点では、β版です。近日中にUpdateします】<br>

---


### 使い方 ###
後で更新します。<br>
1. Wifi接続をスマホアプリ: Esp touchから設定
[![手順01](images/x_stackchan01.jpg)](https://x.com/UtaAoya/status/1794857755968508118)
	- 参考：Esp touchについて
	https://lab.seeed.co.jp/entry/2022/10/17/120000

2. 各種 APIキーをWebブラウザから設定
[![手順01](images/x_stackchan02.jpg)](https://x.com/UtaAoya/status/1794864738746478920)

<br>
<br>
<br>