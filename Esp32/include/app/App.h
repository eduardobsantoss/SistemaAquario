#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <FirebaseClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// ====== Identificação / OTA ======
#define HOSTNAME_DEFAULT "aquario-esp32-devkitc"
#define OTA_PORT_DEFAULT 3232

// ====== LOG / HEARTBEAT ======
#define LOG_HEARTBEAT 1

// ====== Pinos ======
#define ONE_WIRE_BUS 4
#define PH_ADC_PIN 34
#define PIN_RELAY_HEATER 23
#define RELAY_ACTIVE_LOW 1
#define PIN_FLOAT_SWITCH 18
#define PIN_RELAY_WATERFALL 22
#define PIN_BUZZER 19
#define BUZZER_ENABLE 1
#define BUZZER_ACTIVE_LEVEL HIGH
#define BUZZER_ON_MS 120
#define BUZZER_OFF_MS 380
#define BUZZER_GROUP_PAUSE_MS 2000
#define I2C_SDA 16
#define I2C_SCL 17
#define PIN_BTN 21
#define BTN_DEBOUNCE_MS 50

// ====== ADC ======
#define ADC_MAX_COUNTS 4095.0
#define ADC_VREF 3.3

// ====== Boia/cascata: antirruído ======
#define T_LOW_CONFIRM_MS 2000
#define T_HIGH_CONFIRM_MS 1000

// ====== Alimentador ======
#define FEED_IN1 25
#define FEED_IN2 26
#define FEED_IN3 27
#define FEED_IN4 33
#define FEED_STEP_INTERVAL_MS 2
#define MAX_PORTIONS_PER_EVENT 2
#define STEPS_PER_REV 4096

class App
{
public:
    void begin();
    void tick();

private:
    // ====== Firebase (v1.5.11) ======
    WiFiClientSecure sslClient;
    network_config_data net{};
    AsyncClientClass aClient{sslClient, net};
    FirebaseApp app;
    RealtimeDatabase Database;
    UserAuth *pAuth{nullptr};

    static void processData(AsyncResult &aResult);
    inline bool fbReady() { return app.ready(); }

    // ====== Wi-Fi / OTA / NTP ======
    void connectWiFi();
    void syncTime();
    void setupOTA();

    // ====== Relés ======
    inline void relayOn(int pin)
    {
#if RELAY_ACTIVE_LOW
        digitalWrite(pin, LOW);
#else
        digitalWrite(pin, HIGH);
#endif
    }
    inline void relayOff(int pin)
    {
#if RELAY_ACTIVE_LOW
        digitalWrite(pin, HIGH);
#else
        digitalWrite(pin, LOW);
#endif
    }

    // ====== DS18B20 ======
    OneWire oneWire{ONE_WIRE_BUS};
    DallasTemperature sensors{&oneWire};

    // ====== Controle térmico ======
    float T_SET = 26.0f;
    float T_HYST = 1.0f;
    float T_MIN_SAFE = 15.0f;
    float T_MAX_SAFE = 35.0f;
    inline float T_MIN_ON() const { return T_SET - T_HYST; }
    inline float T_MAX_OFF() const { return T_SET + T_HYST; }
    bool heaterOn = false;
    const unsigned long MIN_SWITCH_MS = 30000;
    unsigned long lastSwitchMs = 0;

    // ====== pH ======
    const float M_PH = -5.77f;
    const float B_PH = 22.00f;
    bool phInit = false;
    float emaV = 0.0f;
    const float ALPHA = 0.15f;
    const float PH_MIN = 0.0f, PH_MAX = 14.0f;

    // ====== Agregação (5 min) ======
    const unsigned long SAMPLE_MS = 25000;
    const unsigned long UPLOAD_MS = 300000;
    unsigned long lastSampleTemp = 0, lastUploadTemp = 0;
    double sumTemp = 0.0;
    int nTemp = 0;
    unsigned long lastSamplePH = 0, lastUploadPH = 0;
    double sumPH = 0.0;
    int nPH = 0;
    String lastAvgTempKey;
    float lastAvgTempSent = NAN;

    String lastAvgPhKey;
    float lastAvgPhSent = NAN;

    // ====== Estado boia/cascata ======
    bool waterOk = true;
    bool waterfallOn = true;

    inline int readFloatRaw() { return digitalRead(PIN_FLOAT_SWITCH); }
    inline int readFloatAsBoia() { return (readFloatRaw() == LOW) ? 1 : 0; }
    void setWaterfall(bool on);

    // ====== Buzzer ======
    inline void buzzerOn() { digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LEVEL); }
    inline void buzzerOff() { digitalWrite(PIN_BUZZER, !BUZZER_ACTIVE_LEVEL); }
    void buzzerPatternWaterLow();

    // ====== LCD / Botão ======
    enum class Screen : uint8_t
    {
        RESUMO = 0,
        DETALHE = 1
    };
    Screen currentScreen = Screen::RESUMO;
    uint8_t lcdAddr = 0x27;
    LiquidCrystal_I2C lcd{lcdAddr, 16, 2};
    uint32_t tLastLcd = 0, tLastRotate = 0;
    bool autoRotate = false;
    const uint32_t ROTATE_EVERY_MS = 5000;
    const uint32_t LCD_REFRESH_MS = 400;
    float gLastTempC = NAN;
    float gLastPH = NAN;

    uint8_t scanI2C();
    void initLCD();
    void lcdClearLine(uint8_t row);
    void drawResumo();
    void drawDetalhe();
    void updateLCD();
    void handleButton();

    // ====== Alimentador ======
    bool feederBusy = false;
    long feederTargetSteps = 0;
    long feederRemainingSteps = 0;
    uint8_t feederStepIndex = 0;
    unsigned long tLastStep = 0;
    uint64_t lastFeedTs = 0;
    unsigned long tLastFeedNowPoll = 0;
    int FEED_STEPS_PER_PORTION = 4096;
    const uint64_t FEED_INTERVAL_MS = 12ULL * 60ULL * 60ULL * 1000ULL;

    void feederApplyStep(uint8_t idx);
    void feederReleaseCoils();
    void feederBeginMove(long stepsCW);
    void feederRun();
    bool feederRequest(uint8_t portions);

    // ====== Firebase publishers / logs ======
    static uint64_t epoch_ms();
    void publicarHeater(bool on);
    void publicarWaterOk(bool ok);
    void publicarWaterfall(bool on);
    void logHeaterDecision(float tC, bool newState, float onThr, float offThr, const char *reason);
    void publicarLastSeen();
    void setupFirebaseListeners();

    // ====== Estado Firebase ======
    static App *instance;
    bool fb_ready_notified = false;
    bool pending_heater_state_publish = true;
    bool feederFbInitDone = false;
    bool fb_need_reauth = false;
    uint32_t fb_cooldown_until = 0;
    int fb_last_err = 0;
    // >>> HEARTBEAT (last_seen)
    uint32_t lastSeenAt = 0;
    bool fb_was_ready = false;
};