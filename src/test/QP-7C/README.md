# QP-7C_RP2040_CAT
## QP-7C（CRkits共同購入プロジェクト）の改造　III （QP-7C_RP2040のCAT対応）

 ”QP-7C（CRkits共同購入プロジェクト）の改造 II ”(https://github.com/je1rav/QP-7C_RP2040/) の続きですが，USB AudioとSerial通信の同時使用のため．Arduino IDEでなく”pico-sdk”を直接使用し，C言語で作成しましたので別レポジトリーにしました．  
まだ問題があるのですが，これ以上の対応策がよくわからないので，とりあえず公開します．  
ハードウェアは，https://github.com/je1rav/QP-7C_RP2040/ をご参照ください．  

### pico-SDK使用の経緯
 ”QP-7C（CRkits共同購入プロジェクト）の改造 II ” https://github.com/je1rav/QP-7C_RP2040/ では，pico用ボードマネジャー”Arduino Mbed OS RP2040 Boards”を使用し，Arduino IDEで開発しました．  
(もともとは，Seeed XIAO RP2040用のMbed対応ボードマネージャーを使用: このボードマネージャーは現在開発中止)  
現在，Arduino Mbed OSでは，USB AudioとUSB Seralの同時使用に問題があるようで，少なくともWindowsではどちらか一方しか使用できません．  
”Arduino Core for mbed enabled devices”のgithubでは，issue中の”PluggableUSBAudio not working #546”でこのことが議論されています．  
実際に試したところ,MacOSでは同時使用も可能でした（多分Linuxも大丈夫）が，WindowsやAndroidでは現時点ではダメそうです．  

一方，RP2040でUSBを使うには，TinyUSBがあります．  
しかし，Arduino IDE用の”Adafruit TinyUSB Library for Arduino”は，現時点ではUSB Audioをサポートしていません．  
もしもサポートしていれば，Seeed XIAO RP2040もサポートしているボードマネジャー”Raspberry Pi Pico / RP2040”上で開発できたのですが．  
というわけで，現時点でTinyUSBをフルバージョンで使うには，pico-sdkを直接使用するしかありません．  
Arduino IDEでのプログラム言語はC++ですが，pico-sdkは基本的にC言語で書かれています．  
このため，C言語を使って，USB AudioとSerial通信を同時使用したトランシーバープログラムを作成することにしました．  

Si5351の制御には，NT7Sの”Si5351 Library for avr-gcc” https://github.com/NT7S/Si5351 のi2c通信部分を書き換えて使用しました．  
USB AudioとUSB CDC(Serial)の同時使用は，Ángel Molina Núñezの”rpi-pico-cdc-uac2” https://github.com/angmolin/rpi-pico-cdc-uac2 を参考にしました．  
USB Audioのプログラムは，TinyUSBのexamplesの中の”uac2-headset”に送信および受信部分を分離したコードを追加して使用し，main.cとは別のファイル”usb_Audio.c”に分けています．  
NeoPixelの制御は，pico-examplesにある”pico-examples/pio/ws2812/“を参考にしました．  
CAT制御は，基本的にhttps://github.com/je1rav/QP-7C と同じ中身で，Kenwood TS-2000をシミュレートしています．  
PCに接続すると，オーディオデバイスとして”TinyUSB headset”が現れ，また，Comポートにも新しいUSB Serial Deviceが現れます．  

Pushスイッチによる周波数切り替えには，少し長押し(0.7秒)が必要です．  
選択した周波数の区別はNeoPixelの色で行い，CAT制御時にはNeoPixelが消灯します．  
Xiao RP2040のオンボードLEDを使用し，受信時には緑が点灯し，送信時には赤が点灯するようにしています．  
ハングアップした場合には，watchdogタイマーを使用して1秒でリセットが起こります．  
しかし，リセットが起こると，PCとの接続が一旦切れますので，ソフトを立ち上げ直さないと動きません．  
もしかすると，一度USBケーブルを抜き差ししないといけないかもしれません．  

### Windowsでの使用の注意点
Windows上でwsjt-xと共に使用する場合しか検証していませんが，サウンド設定を以下のようにしておかないと4回目の送信で必ずハングアップしました．  
TinyUSBのexamplesの中の”uac2-headset”でも同様なことが起きましたので，TinyUSBの方の問題かも知れません．  
もしかしたら，TinyUSBのgithubのissue “UAC2 Headset example fails after some playback time #1728”が関係あるかもしれません．  
macOSでは，問題なく動作しました．  
#### Windowsのサウンド設定方法  
	録音デバイスで，TinyUSB headsetのMicrophonesを”既定のデバイス”に設定する．
	再生デバイスで，TinyUSB headsetのHeadphonesを”既定のデバイス”に設定する．
	再生デバイスでTinyUSB headsetのHeadphonesを”既定のデバイス”に設定すると，送信時に変調音がpcのスピーカーから聞こえますので，音量を下げるか0にしておいて下さい．
	wsjt-xの出力は最大にしておきます．

### ビルドの方法
ビルド環境設定の方法やCMakeLists.txtの書き方などがよくわからないので，とりあえずビルドできた方法のメモです．  
単独プロジェクトにする方法が，よく分からなかったので，例題プログラム群にお邪魔する形で，ビルドしました．  
正統なやり方ではないと思いますが，手軽に開発するには便利だと思いました．  
私はMacを使ってビルドしましたが，おそらく他のOSでも大丈夫だと思います．  
現時点でのソースプログラムはダイレクトコンバージョン用です．  
スーパーヘテロダイン用にするには， "main.c"の41行目//#define Superheterodyneのコメントアウト”//"を消して，#define文を有効にしてください．  
#### 1．pico-sdkのプログラムをビルドできるように環境整備する．  
	インターネットなどに情報がたくさんありますので，それをもとに構築しました．  
	私は，VSCodeで開発できるようにしました．  
#### 2．pico-sdkにはtinyUSBの例題プログラムも付属しているので，pico-sdk内のtinyUSBの例題プログラムをビルドできるようにする．  
	”pico-sdk/lib/tinyUSB/examples/”の下にtinyUSBの例題プログラムが収録されていますが，最初の段階ではビルドするようにはなっていません．  
	そこで，pico-sdkディレクトリー直下にある”CMakeLists.txt”ファイルを変更します．  
	add_subdirectory(tools)  
	add_subdirectory(src)  
	の下に次の行を追加します．  
	
	add_subdirectory(lib/tinyusb/examples/device)   
	
	そうすると，lib/tinyusb/examples/device/ディレクトリー内の種々の例題プログラムもビルドされるようになります．  
	これらの例題プログラムはRP2040側をUSBデバイスとして使用する様々な例を示しており，今回はこの中の”uac2-headset”を元にして作成しました．  
#### 3．tinyUSBのexamples/device/ディレクトリーにqp7c_rp2040_catのプログラムをコピーする．  
	examples/device/の中にフォルダごとコピーします．  
	こうすることで，他の例題プログラムと同様に本プログラムをビルドするための準備ができます．  
#### 4．本プログラムがビルドされるようにexamples/device/中のCMakeLists.txt”ファイルを変更する．  
	tinyUSBのexamples/device/ディレクトリーにある”CMakeLists.txt”ファイルを変更します．  
	このファイルには，ビルドするプログラムが列挙されています．  
	ファイルの最後に次の行を追加します．  
  
	family_add_subdirectory(qp7c_rp2040_cat)  
  
	これで，qp7c_rp2040_catがビルドされるようになります．  
#### 5．ビルドします．
	うまくいけば，ビルドされたファイルは”pico-sdk/build/lib/tinyusb/examples/device/qp7c_rp2040_catディレクトリー内にできているはずです．  
	使用する実行ファイルは，”qp7c_rp2040_cat.uf2”です．  
#### 6．RP2040を”bootsel”ボタンを押しながらリセットすると，プログラム書き込みモードで立ち上げり，PCに外部ドライブとして現れます．  
	”qp7c_rp2040_cat.uf2”をDrag & Dropしてインストール完了です．  
#### 7.　私がビルドした実行ファイルもアップロードしておきます．  si5351の発振周波数の校正やIF通過周波数などの調整を行うには，ソースファイルを変更して，ビルドしてください．  
	qp7c_rp2040_dc_cat.uf2は，ダイレクトコンバージョン版用,  
	qp7c_rp2040_super450_cat.uf2は，スーパーヘテロダイン版 (IFフィルター450 kHz)用,  
	qp7c_rp2040_super455_cat.uf2は，スーパーヘテロダイン版 (IFフィルター455 kHz)用です．  
	
