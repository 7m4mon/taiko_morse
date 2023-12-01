//====================================================================
//  PS2版太鼓の達人コントローラーでモールス信号を送信する
//  2023.12.01 7M4MON
//  ドン＝長点、カッ＝短点、セレクト＝遅く、スタート＝速く
//====================================================================

// Gyokimae氏の自作ライブラリ(PSコントローラ用)
// http://pspunch.com/pd/article/arduino_lib_gpsx.html
#include <GPSXClass.h>
#include <MsTimer2.h>
#include <EEPROM.h>

//====================================================================
//  各種定義
//====================================================================
#define PIN_KEY_OUT 13

#define POLLING_INTERVAL_MS 5
#define DOT_LENGTH_MIN 6    // 5ms*6 = 30ms → 40WPM
#define DOT_LENGTH_MAX 48   // 5ms*48 = 240ms → 5WPM
#define DOT_LENGTH_DEF 20   // 5ms*20 = 100ms → 12WPM　(昔の１アマ相当)

uint16_t keydown_time;
uint16_t dot_length;

typedef struct
{
    bool button_dl; //←
    bool button_dr; //◯
    bool button_kl; //L1
    bool button_kr; //R1
    bool button_select;
    bool button_start; 
} TaikoState_t;

//====================================================================
//  キーイング関連の関数
//====================================================================

// タイマー割り込みで呼び出されてコントローラの状態を更新する。
TaikoState_t get_taiko_state(){
    PSX.updateState(PSX_PAD1);

    TaikoState_t current_taiko_state;
    current_taiko_state.button_dl = (bool)PRESSED_LEFT(PSX_PAD1);
    current_taiko_state.button_dr = (bool)PRESSED_CIRCLE(PSX_PAD1);
    current_taiko_state.button_kl = (bool)PRESSED_L1(PSX_PAD1);
    current_taiko_state.button_kr = (bool)PRESSED_R1(PSX_PAD1);
    current_taiko_state.button_select = (bool)PRESSED_SELECT(PSX_PAD1);
    current_taiko_state.button_start = (bool)PRESSED_START(PSX_PAD1);

    return current_taiko_state;
}

// タイマー割り込みで呼び出されてキーイングを行う。
void interrpt_proc(){
    TaikoState_t taiko_state = get_taiko_state();
    if (keydown_time == 0){
        if (taiko_state.button_kl || taiko_state.button_kr) keydown_time = dot_length;
        if (taiko_state.button_dl || taiko_state.button_dr) keydown_time = dot_length * 3;
        if (taiko_state.button_start && (dot_length > DOT_LENGTH_MIN)) {dot_length--; keydown_time = dot_length; EEPROM.write(0x00, (uint8_t)dot_length);}
        if (taiko_state.button_select && (dot_length < DOT_LENGTH_MAX)) {dot_length++; keydown_time = dot_length; EEPROM.write(0x00, (uint8_t)dot_length);}
        if (taiko_state.button_start && taiko_state.button_select) {dot_length = DOT_LENGTH_DEF; keydown_time = dot_length; EEPROM.write(0x00, (uint8_t)dot_length);}    //同時押しでデフォルト値
        if (keydown_time != 0) {digitalWrite(PIN_KEY_OUT, HIGH); Serial.println(keydown_time * POLLING_INTERVAL_MS);}
    }else{
        keydown_time--;
        if (keydown_time == 0) digitalWrite(PIN_KEY_OUT, LOW);
    }
}

//====================================================================
//  初期化処理
//====================================================================
void setup()
{
    // デバッグ用シリアルを開始
    Serial.begin(115200);

    // PSコントローラライブラリの初期化
    PSX.mode(PSX_PAD1, MODE_DIGITAL, MODE_LOCK);
    PSX.motorEnable(PSX_PAD1, MOTOR1_DISABLE, MOTOR2_DISABLE);

    //5msごとにコントローラーを読み取り
    MsTimer2::set(POLLING_INTERVAL_MS, interrpt_proc); // 
    MsTimer2::start();

    // 各種初期化
    digitalWrite(PIN_KEY_OUT, LOW);
    keydown_time = 0;
    dot_length = (uint16_t) EEPROM.read(0x00);
    if ((dot_length < DOT_LENGTH_MIN) || (dot_length > DOT_LENGTH_MAX) ) dot_length = DOT_LENGTH_DEF;
}

//====================================================================
//  メインループ　（タイマー割り込み処理だけなので何もしない）
//====================================================================
void loop()
{

}

