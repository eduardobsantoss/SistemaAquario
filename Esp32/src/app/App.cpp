#include <Arduino.h>
#include <math.h>
#include "config/Secrets.h"
#include "app/App.h"

App *App::instance = nullptr;

// ===== Flags locais para modos =====
static bool heaterModeAuto = true;
static bool waterfallModeAuto = true;

// ===== Pendências de publicação para evitar reentrância no callback =====
static volatile bool pending_publish_heater = false;
static volatile bool pending_publish_waterfall = false;
static volatile bool pending_reset_feednow = false;

// Callback "mudo" apenas para logar resultado das operações de escrita
static void fbAck(AsyncResult &res)
{
    if (res.isError())
    {
        Serial.printf("[FB][%s] write ERROR %d: %s\n",
                      res.uid().c_str(),
                      res.error().code(),
                      res.error().message().c_str());
    }
}

// ================= Firebase callback =================
void App::processData(AsyncResult &aResult)
{
    if (aResult.isError())
    {
        const int code = aResult.error().code();
        Serial.printf("[FB][%s] ERROR %d: %s\n",
                      aResult.uid().c_str(),
                      code,
                      aResult.error().message().c_str());

        // Se token ficou inválido/expirou → reautentica com pequeno backoff
        if (code == 401)
        {
            App::instance->fb_need_reauth = true;
            App::instance->fb_cooldown_until = millis() + 2000;
            App::instance->fb_last_err = code;
            Serial.println("[FB] 401 detectado → agendando reauth em 2s");
        }
        return;
    }

    // ===== Listener de MODO do HEATER =====
    if (aResult.uid() == "heater_mode_listener")
    {
        RealtimeDatabaseResult &val = aResult.to<RealtimeDatabaseResult>();
        String mode = val.to<String>();
        bool wasAuto = heaterModeAuto;
        heaterModeAuto = (mode != "manual");

        if (wasAuto != heaterModeAuto)
        {
            Serial.printf("[FB] Heater modo alterado: %s\n", heaterModeAuto ? "AUTO" : "MANUAL");
        }
        return;
    }

    // ===== Listener de turn_on_now do HEATER =====
    if (aResult.uid() == "heater_cmd_listener")
    {
        RealtimeDatabaseResult &val = aResult.to<RealtimeDatabaseResult>();
        bool cmd = val.to<bool>();

        static bool lastHeaterCmd = false;

        if (!heaterModeAuto)
        {
            if (cmd != lastHeaterCmd)
            {
                App *app = App::instance;

                if (cmd)
                {
                    app->relayOn(PIN_RELAY_HEATER);
                    app->heaterOn = true;
                    pending_publish_heater = true;
                    Serial.println("[CMD] Heater: LIGADO (manual via Firebase)");
                }
                else
                {
                    app->relayOff(PIN_RELAY_HEATER);
                    app->heaterOn = false;
                    pending_publish_heater = true;
                    Serial.println("[CMD] Heater: DESLIGADO (manual via Firebase)");
                }
            }
        }

        lastHeaterCmd = cmd;
        return;
    }

    // ===== Listener de MODO da WATERFALL =====
    if (aResult.uid() == "waterfall_mode_listener")
    {
        RealtimeDatabaseResult &val = aResult.to<RealtimeDatabaseResult>();
        String mode = val.to<String>();
        bool wasAuto = waterfallModeAuto;
        waterfallModeAuto = (mode != "manual");

        if (wasAuto != waterfallModeAuto)
        {
            Serial.printf("[FB] Waterfall modo alterado: %s\n", waterfallModeAuto ? "AUTO" : "MANUAL");
        }
        return;
    }

    // ===== Listener de turn_on_now da WATERFALL =====
    if (aResult.uid() == "waterfall_cmd_listener")
    {
        RealtimeDatabaseResult &val = aResult.to<RealtimeDatabaseResult>();
        bool cmd = val.to<bool>();

        static bool lastWfCmd = false;

        if (!waterfallModeAuto)
        {
            if (cmd != lastWfCmd)
            {
                App *app = App::instance;

                if (cmd)
                {
                    app->setWaterfall(true);
                    Serial.println("[CMD] Waterfall: LIGADA (manual via Firebase)");
                }
                else
                {
                    app->setWaterfall(false);
                    Serial.println("[CMD] Waterfall: DESLIGADA (manual via Firebase)");
                }
            }
        }

        lastWfCmd = cmd;
        return;
    }

    // ===== Listener de feed_now do FEEDER =====
    if (aResult.uid() == "feeder_cmd_listener")
    {
        RealtimeDatabaseResult &val = aResult.to<RealtimeDatabaseResult>();
        bool want = val.to<bool>();
        if (want)
        {
            App *app = App::instance;
            bool ok = app->feederRequest(1);

            pending_reset_feednow = true;

            if (ok)
            {
                Serial.println("[FEEDER] feed_now acionado via Firebase");
            }
            else
            {
                Serial.println("[FEEDER] feed_now solicitado, mas ocupado ou nivel baixo");
            }
        }
        return;
    }
}

// ================= Wi-Fi / NTP / OTA =================
void App::connectWiFi()
{
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
        Serial.print(".");
    }
    Serial.printf("\nWi-Fi OK. IP: %s\n", WiFi.localIP().toString().c_str());
    sslClient.setInsecure();
}

void App::syncTime()
{
    const long GMT_OFFSET_SEC = -3 * 3600; // America/Sao_Paulo
    const int DST_OFFSET_SEC = 0;
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");

    time_t now = time(nullptr);
    Serial.print("Sincronizando NTP");
    int tries = 0;
    while (now < 1700000000 && tries < 60)
    {
        delay(250);
        Serial.print(".");
        now = time(nullptr);
        tries++;
    }
    Serial.println();
}

void App::setupOTA()
{
    ArduinoOTA.setHostname(HOSTNAME_DEFAULT);
    ArduinoOTA.setPort(OTA_PORT_DEFAULT);
    ArduinoOTA.onStart([]()
                       { Serial.println("\nOTA: start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nOTA: end"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("OTA: %u%%\r", (progress * 100U) / total); });
    ArduinoOTA.onError([](ota_error_t error)
                       { Serial.printf("OTA: erro %u\n", error); });

    ArduinoOTA.begin();
    Serial.printf("OTA pronto. Host: %s:%d\n", HOSTNAME_DEFAULT, OTA_PORT_DEFAULT);

    if (!MDNS.begin(HOSTNAME_DEFAULT))
    {
        Serial.println("mDNS falhou");
    }
    else
    {
        MDNS.addService("arduino", "tcp", OTA_PORT_DEFAULT);
        MDNS.addServiceTxt("arduino", "tcp", "board", "esp32");
        MDNS.addServiceTxt("arduino", "tcp", "auth", "no");
        Serial.println("mDNS/OTA anunciados");
    }
}

// ================= Helpers =================
uint64_t App::epoch_ms()
{
    time_t s = time(nullptr);
    if (s > 100000)
        return (uint64_t)s * 1000ULL;
    return (uint64_t)millis();
}

void App::buzzerPatternWaterLow()
{
#if BUZZER_ENABLE
    static uint8_t step = 0;
    static unsigned long t0 = 0;
    unsigned long now = millis();

    switch (step)
    {
    case 0:
        buzzerOn();
        t0 = now;
        step = 1;
        break;
    case 1:
        if (now - t0 >= BUZZER_ON_MS)
        {
            buzzerOff();
            t0 = now;
            step = 2;
        }
        break;
    case 2:
        if (now - t0 >= BUZZER_OFF_MS)
        {
            buzzerOn();
            t0 = now;
            step = 3;
        }
        break;
    case 3:
        if (now - t0 >= BUZZER_ON_MS)
        {
            buzzerOff();
            t0 = now;
            step = 4;
        }
        break;
    case 4:
        if (now - t0 >= BUZZER_OFF_MS)
        {
            buzzerOn();
            t0 = now;
            step = 5;
        }
        break;
    case 5:
        if (now - t0 >= BUZZER_ON_MS)
        {
            buzzerOff();
            t0 = now;
            step = 6;
        }
        break;
    case 6:
        if (now - t0 >= BUZZER_GROUP_PAUSE_MS)
        {
            step = 0;
        }
        break;
    }
#else
    buzzerOff();
#endif
}

void App::setWaterfall(bool on)
{
    if (RELAY_ACTIVE_LOW)
        digitalWrite(PIN_RELAY_WATERFALL, on ? LOW : HIGH);
    else
        digitalWrite(PIN_RELAY_WATERFALL, on ? HIGH : LOW);

    waterfallOn = on;
    pending_publish_waterfall = true;
}

// ================= LCD =================
uint8_t App::scanI2C()
{
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            found = addr;
            break;
        }
    }
    return found;
}

void App::initLCD()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);
    uint8_t found = scanI2C();
    if (found != 0)
        lcdAddr = found;
    lcd = LiquidCrystal_I2C(lcdAddr, 16, 2);
    lcd.init();
    lcd.backlight();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LCD I2C @0x");
    char buf[5];
    sprintf(buf, "%02X", lcdAddr);
    lcd.print(buf);
    lcd.setCursor(0, 1);
    lcd.print("Inicializando...");
    delay(500);
    lcd.clear();
}

void App::lcdClearLine(uint8_t row)
{
    lcd.setCursor(0, row);
    lcd.print("                ");
}

void App::drawResumo()
{
    char l1[17], l2[17];
    if (isfinite(gLastTempC) && isfinite(gLastPH))
        snprintf(l1, sizeof(l1), "T:%4.1fC pH:%4.2f", gLastTempC, gLastPH);
    else if (isfinite(gLastTempC))
        snprintf(l1, sizeof(l1), "T:%4.1fC pH:--.--", gLastTempC);
    else if (isfinite(gLastPH))
        snprintf(l1, sizeof(l1), "T:--.-C pH:%4.2f", gLastPH);
    else
        snprintf(l1, sizeof(l1), "T:--.-C pH:--.--");

    snprintf(l2, sizeof(l2), "HTR:%s", heaterOn ? "ON " : "OFF");

    lcdClearLine(0);
    lcdClearLine(1);
    lcd.setCursor(0, 0);
    lcd.print(l1);
    lcd.setCursor(0, 1);
    lcd.print(l2);
}

void App::drawDetalhe()
{
    char l1[17], l2[17];
    snprintf(l1, sizeof(l1), "CASC:%s", waterfallOn ? "ON " : "OFF");
    snprintf(l2, sizeof(l2), "NIV:%s", waterOk ? "OK " : "BAIX");

    lcdClearLine(0);
    lcdClearLine(1);
    lcd.setCursor(0, 0);
    lcd.print(l1);
    lcd.setCursor(0, 1);
    lcd.print(l2);
}

void App::updateLCD()
{
    switch (currentScreen)
    {
    case Screen::RESUMO:
        drawResumo();
        break;
    case Screen::DETALHE:
        drawDetalhe();
        break;
    }
}

void App::handleButton()
{
    bool reading = digitalRead(PIN_BTN);
    uint32_t now = millis();

    static bool lastBtn = HIGH;
    static uint32_t tBtnChanged = 0;

    if (reading != lastBtn)
        tBtnChanged = now;
    if ((now - tBtnChanged) > BTN_DEBOUNCE_MS)
    {
        static bool stable = HIGH;
        if (reading != stable)
        {
            stable = reading;
            if (stable == LOW)
            {
                currentScreen = (currentScreen == Screen::RESUMO) ? Screen::DETALHE : Screen::RESUMO;
                updateLCD();
            }
        }
    }
    lastBtn = reading;
}

// ================= Feeder =================
void App::feederApplyStep(uint8_t idx)
{
    switch (idx & 0x07)
    {
    case 0:
        digitalWrite(FEED_IN1, 1);
        digitalWrite(FEED_IN2, 0);
        digitalWrite(FEED_IN3, 0);
        digitalWrite(FEED_IN4, 0);
        break;
    case 1:
        digitalWrite(FEED_IN1, 1);
        digitalWrite(FEED_IN2, 1);
        digitalWrite(FEED_IN3, 0);
        digitalWrite(FEED_IN4, 0);
        break;
    case 2:
        digitalWrite(FEED_IN1, 0);
        digitalWrite(FEED_IN2, 1);
        digitalWrite(FEED_IN3, 0);
        digitalWrite(FEED_IN4, 0);
        break;
    case 3:
        digitalWrite(FEED_IN1, 0);
        digitalWrite(FEED_IN2, 1);
        digitalWrite(FEED_IN3, 1);
        digitalWrite(FEED_IN4, 0);
        break;
    case 4:
        digitalWrite(FEED_IN1, 0);
        digitalWrite(FEED_IN2, 0);
        digitalWrite(FEED_IN3, 1);
        digitalWrite(FEED_IN4, 0);
        break;
    case 5:
        digitalWrite(FEED_IN1, 0);
        digitalWrite(FEED_IN2, 0);
        digitalWrite(FEED_IN3, 1);
        digitalWrite(FEED_IN4, 1);
        break;
    case 6:
        digitalWrite(FEED_IN1, 0);
        digitalWrite(FEED_IN2, 0);
        digitalWrite(FEED_IN3, 0);
        digitalWrite(FEED_IN4, 1);
        break;
    case 7:
        digitalWrite(FEED_IN1, 1);
        digitalWrite(FEED_IN2, 0);
        digitalWrite(FEED_IN3, 0);
        digitalWrite(FEED_IN4, 1);
        break;
    }
}

void App::feederReleaseCoils()
{
    digitalWrite(FEED_IN1, 0);
    digitalWrite(FEED_IN2, 0);
    digitalWrite(FEED_IN3, 0);
    digitalWrite(FEED_IN4, 0);
}

void App::feederBeginMove(long stepsCW)
{
    feederTargetSteps = stepsCW;
    feederRemainingSteps = stepsCW;
    feederStepIndex = 0;
    feederBusy = true;
    tLastStep = 0;

    if (fbReady())
        Database.set<bool>(aClient, "/aquario/status/feeder/busy", true, processData, "RTDB_Status_feeder_busy");
}

void App::feederRun()
{
    if (!feederBusy)
        return;
    const unsigned long now = millis();
    if (now - tLastStep < FEED_STEP_INTERVAL_MS)
        return;

    feederApplyStep(feederStepIndex++);
    feederRemainingSteps--;
    tLastStep = now;

    if (feederRemainingSteps <= 0)
    {
        feederBusy = false;
        feederReleaseCoils();
        lastFeedTs = epoch_ms();

        if (fbReady())
        {
            Database.set<uint64_t>(aClient, "/aquario/status/feeder/last_ts", lastFeedTs, processData, "RTDB_Status_feeder_ts");
            Database.set<bool>(aClient, "/aquario/status/feeder/busy", false, processData, "RTDB_Status_feeder_busy");
        }
        Serial.println("[FEEDER] Concluido");
    }
}

bool App::feederRequest(uint8_t portions)
{
    if (portions == 0 || portions > MAX_PORTIONS_PER_EVENT)
        return false;
    if (feederBusy)
        return false;
    if (!waterOk)
    {
        Serial.println("[FEEDER] BLOQUEADO: nivel baixo de agua");
        return false;
    }
    long steps = (long)FEED_STEPS_PER_PORTION * (long)portions;
    feederBeginMove(steps);
    Serial.printf("[FEEDER] Iniciando: %u porcao(oes), %ld passos\n", portions, steps);
    return true;
}

// ================= Firebase publishers/logs =================
void App::publicarHeater(bool on)
{
    pending_publish_heater = true;

    pending_heater_state_publish = false;
}

void App::publicarWaterOk(bool ok)
{
    if (!fbReady())
        return;
    Database.set<bool>(aClient, "/aquario/float/water_ok", ok, processData, "RTDB_Float_WaterOk");
}

void App::publicarWaterfall(bool on)
{
    pending_publish_waterfall = true;
}

void App::logHeaterDecision(float tC, bool newState, float onThr, float offThr, const char *reason)
{
    Serial.printf("[CTRL] Aquecedor: %s | t=%.2f°C | Liga<%.2f  Desliga>%.2f | %s\n",
                  newState ? "ON " : "OFF", tC, onThr, offThr, reason);
}

void App::publicarLastSeen()
{
    if (!fbReady())
        return;
    Database.set<uint64_t>(aClient, "/aquario/status/last_seen", (uint64_t)epoch_ms(), processData, "RTDB_LastSeen");
}

// ================= Firebase Listeners Setup =================
void App::setupFirebaseListeners()
{
    if (!fbReady())
        return;

    Serial.println("[FB] Configurando listeners...");

    Database.get(aClient, "/aquario/controle/heater/mode", processData, false, "heater_mode_listener");

    Database.get(aClient, "/aquario/controle/heater/turn_on_now", processData, false, "heater_cmd_listener");

    Database.get(aClient, "/aquario/controle/waterfall/mode", processData, false, "waterfall_mode_listener");

    Database.get(aClient, "/aquario/controle/waterfall/turn_on_now", processData, false, "waterfall_cmd_listener");

    Database.get(aClient, "/aquario/controle/feeder/feed_now", processData, false, "feeder_cmd_listener");

    Serial.println("[FB] Listeners iniciais solicitados");
}

// ================= Public: begin/tick =================
void App::begin()
{
    App::instance = this;
    Serial.begin(115200);
    delay(200);
    Serial.println("\nBoot ESP32 + DS18B20 + Sensor pH + FirebaseClient + OTA + HeaterCtrl + FloatSwitch + LCD + Feeder");

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    pinMode(PIN_RELAY_HEATER, OUTPUT);
    relayOff(PIN_RELAY_HEATER);
    heaterOn = false;

    pinMode(PIN_RELAY_WATERFALL, OUTPUT);
    setWaterfall(true);

    pinMode(PIN_FLOAT_SWITCH, INPUT_PULLUP);
    waterOk = (readFloatAsBoia() == 1);
    publicarWaterOk(waterOk);

    pinMode(PIN_BUZZER, OUTPUT);
    buzzerOff();

    pinMode(PIN_BTN, INPUT_PULLUP);

    pinMode(FEED_IN1, OUTPUT);
    pinMode(FEED_IN2, OUTPUT);
    pinMode(FEED_IN3, OUTPUT);
    pinMode(FEED_IN4, OUTPUT);
    feederReleaseCoils();

    connectWiFi();
    syncTime();
    setupOTA();

    if (pAuth)
    {
        delete pAuth;
        pAuth = nullptr;
    }
    pAuth = new UserAuth(WEB_API_KEY, USER_EMAIL, USER_PASS);
    initializeApp(aClient, app, getAuth(*pAuth), App::processData, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);

    sensors.begin();

    initLCD();
    updateLCD();
    tLastLcd = millis();
    tLastRotate = millis();

    pending_heater_state_publish = true;
}

void App::tick()
{
    app.loop();
    ArduinoOTA.handle();

    // === Flush de pendências de publicação ===
    if (fbReady() && !fb_need_reauth)
    {
        if (pending_publish_heater)
        {
            Database.set<bool>(aClient, "/aquario/controle/heater/state",
                               heaterOn, fbAck, "RTDB_Set_Heater_state");
            pending_publish_heater = false;
        }
        if (pending_publish_waterfall)
        {
            Database.set<bool>(aClient, "/aquario/controle/waterfall/state",
                               waterfallOn, fbAck, "RTDB_Set_Waterfall_state");
            pending_publish_waterfall = false;
        }
        if (pending_reset_feednow)
        {
            Database.set<bool>(aClient, "/aquario/controle/feeder/feed_now",
                               false, fbAck, "RTDB_Reset_FeedNow");
            pending_reset_feednow = false;
        }
    }

    const uint32_t now = millis();

    if (fb_need_reauth && (int32_t)(now - fb_cooldown_until) >= 0)
    {
        Serial.println("[FB] Reauth iniciando...");
        if (!pAuth)
            pAuth = new UserAuth(WEB_API_KEY, USER_EMAIL, USER_PASS);

        initializeApp(aClient, app, getAuth(*pAuth), App::processData, "reauthTask");
        app.getApp<RealtimeDatabase>(Database);
        Database.url(DATABASE_URL);

        fb_need_reauth = false;
        fb_ready_notified = false;
        fb_cooldown_until = now + 500;
    }

    if (WiFi.status() != WL_CONNECTED)
        connectWiFi();

    if (fbReady() && pending_heater_state_publish)
    {
        publicarHeater(heaterOn);
    }

    static uint8_t initStep = 0;
    static uint32_t lastInitStep = 0;

    if (fbReady() && !feederFbInitDone)
    {
        if (now - lastInitStep >= 500)
        {
            lastInitStep = now;

            switch (initStep)
            {
            case 0:
                Database.set<bool>(aClient, "/aquario/controle/feeder/feed_now", false, processData, "RTDB_Init_FeedNow");
                delay(50);
                break;
            case 1:
                Database.set<bool>(aClient, "/aquario/status/feeder/busy", false, processData, "RTDB_Init_FeederBusy");
                delay(50);
                break;
            case 2:
                Database.set<uint64_t>(aClient, "/aquario/status/feeder/last_ts", 0, processData, "RTDB_Init_FeederTs");
                delay(50);
                break;
            case 3:
                Database.set<int>(aClient, "/aquario/config/feeder/steps_per_portion", FEED_STEPS_PER_PORTION, processData, "RTDB_Init_Steps");
                delay(50);
                break;
            case 4:
                Database.set<int>(aClient, "/aquario/config/feeder/step_interval_ms", (int)FEED_STEP_INTERVAL_MS, processData, "RTDB_Init_Interval");
                delay(50);
                break;
            case 5:
                Database.set<int>(aClient, "/aquario/config/feeder/max_portions_per_event", (int)MAX_PORTIONS_PER_EVENT, processData, "RTDB_Init_MaxPort");
                delay(50);
                break;
            case 6:
                Database.set<const char *>(aClient, "/aquario/controle/heater/mode", "auto", processData, "RTDB_Init_HeaterMode");
                delay(50);
                break;
            case 7:
                Database.set<bool>(aClient, "/aquario/controle/heater/turn_on_now", false, processData, "RTDB_Init_HeaterCmd");
                delay(50);
                break;
            case 8:
                Database.set<const char *>(aClient, "/aquario/controle/waterfall/mode", "auto", processData, "RTDB_Init_WfMode");
                delay(50);
                break;
            case 9:
                Database.set<bool>(aClient, "/aquario/controle/waterfall/turn_on_now", false, processData, "RTDB_Init_WfCmd");
                delay(50);
                feederFbInitDone = true;
                Serial.println("[FB] Nós do feeder + modos criados/atualizados.");
                break;
            }

            initStep++;
        }
    }

    static bool listenersSetup = false;
    static uint32_t nodesCreatedAt = 0;

    if (feederFbInitDone && !listenersSetup)
    {
        if (nodesCreatedAt == 0)
        {
            nodesCreatedAt = now;
        }
        else if (now - nodesCreatedAt >= 2000)
        {
            setupFirebaseListeners();
            listenersSetup = true;
        }
    }

    static uint32_t lastCmdPoll = 0;
    static uint8_t cmdPollIndex = 0;

    if (fbReady() && listenersSetup && (now - lastCmdPoll >= 5000))
    {
        lastCmdPoll = now;

        switch (cmdPollIndex)
        {
        case 0:
            Database.get(aClient, "/aquario/controle/heater/mode", processData, false, "heater_mode_listener");
            break;
        case 1:
            Database.get(aClient, "/aquario/controle/heater/turn_on_now", processData, false, "heater_cmd_listener");
            break;
        case 2:
            Database.get(aClient, "/aquario/controle/waterfall/mode", processData, false, "waterfall_mode_listener");
            break;
        case 3:
            Database.get(aClient, "/aquario/controle/waterfall/turn_on_now", processData, false, "waterfall_cmd_listener");
            break;
        case 4:
            Database.get(aClient, "/aquario/controle/feeder/feed_now", processData, false, "feeder_cmd_listener");
            break;
        }

        cmdPollIndex++;
        if (cmdPollIndex > 4)
            cmdPollIndex = 0;

        delay(100);
    }

    // --- HEARTBEAT: publicar /status/last_seen ---
    if (!fb_was_ready && fbReady())
    {
        publicarLastSeen();
        lastSeenAt = now;
        fb_was_ready = true;
    }
    if (!fbReady())
    {
        fb_was_ready = false;
    }

    if (fbReady() && !fb_need_reauth && (now - lastSeenAt >= 10000))
    {
        publicarLastSeen();
        lastSeenAt = now;
    }

    // (A) BOIA + CASCATA (debounce)
    {
        static uint32_t tMark = 0;
        static int stableLogical = readFloatAsBoia();
        static int lastLogical = stableLogical;

        int nowLogical = readFloatAsBoia();

        if (nowLogical != lastLogical)
        {
            tMark = now;
            lastLogical = nowLogical;
        }

        if (nowLogical != stableLogical)
        {
            uint32_t elapsed = now - tMark;

            bool quedaConfirmada = (nowLogical == 0) && (elapsed >= T_LOW_CONFIRM_MS);
            bool retornoConfirmado = (nowLogical == 1) && (elapsed >= T_HIGH_CONFIRM_MS);

            if (quedaConfirmada || retornoConfirmado)
            {
                stableLogical = nowLogical;
                bool newWaterOk = (stableLogical == 1);

                if (newWaterOk != waterOk)
                {
                    waterOk = newWaterOk;
                    publicarWaterOk(waterOk);

                    if (waterfallModeAuto)
                    {
                        if (waterOk)
                        {
                            setWaterfall(true);
                            Serial.println("[ÁGUA] Nível: OK     | Cascata: LIGADA (auto)");
                        }
                        else
                        {
                            setWaterfall(false);
                            Serial.println("[ÁGUA] Nível: BAIXO  | Cascata: DESLIGADA (auto)");
                        }
                    }
                    else
                    {
                        Serial.printf("[ÁGUA] Nível: %s | Cascata em modo MANUAL (sem ação auto)\n",
                                      waterOk ? "OK" : "BAIXO");
                    }
                }
            }
        }
    }

    // (B) TEMPERATURA + HEATER
    if (now - lastSampleTemp >= SAMPLE_MS)
    {
        lastSampleTemp = now;

        sensors.requestTemperatures();
        float tC = sensors.getTempCByIndex(0);

        if (tC != DEVICE_DISCONNECTED_C)
        {
            sumTemp += tC;
            nTemp++;
            gLastTempC = tC;

#if LOG_HEARTBEAT
            Serial.printf("[AMOSTRA] Temp=%.2f°C | Agua=%s | Cascata=%s | Heater=%s\n",
                          tC,
                          waterOk ? "OK" : "BAIXO",
                          waterfallOn ? "LIGADA" : "DESLIGADA",
                          heaterOn ? "ON" : "OFF");
#endif

            bool changed = false;

            if (!isfinite(tC) || tC < T_MIN_SAFE || tC > T_MAX_SAFE)
            {
                if (heaterOn)
                {
                    relayOff(PIN_RELAY_HEATER);
                    heaterOn = false;
                    changed = true;
                    logHeaterDecision(tC, heaterOn, T_MIN_ON(), T_MAX_OFF(),
                                      heaterModeAuto ? "fail-safe" : "fail-safe (manual)");
                }
                Serial.println("[CTRL] Fail-safe: temperatura fora da faixa. Aquecedor OFF");
            }
            else if (heaterModeAuto)
            {
                bool canSwitch = (now - lastSwitchMs) >= MIN_SWITCH_MS;

                if (!heaterOn && tC < T_MIN_ON() && canSwitch)
                {
                    relayOn(PIN_RELAY_HEATER);
                    heaterOn = true;
                    changed = true;
                    lastSwitchMs = now;
                    logHeaterDecision(tC, heaterOn, T_MIN_ON(), T_MAX_OFF(), "abaixo do limiar");
                }
                else if (heaterOn && tC > T_MAX_OFF() && canSwitch)
                {
                    relayOff(PIN_RELAY_HEATER);
                    heaterOn = false;
                    changed = true;
                    lastSwitchMs = now;
                    logHeaterDecision(tC, heaterOn, T_MIN_ON(), T_MAX_OFF(), "acima do limiar");
                }
            }

            if (changed)
                publicarHeater(heaterOn);
        }
        else
        {
            if (heaterOn)
            {
                relayOff(PIN_RELAY_HEATER);
                heaterOn = false;
                publicarHeater(heaterOn);
                logHeaterDecision(NAN, heaterOn, T_MIN_ON(), T_MAX_OFF(), "sensor desconectado");
            }
            Serial.println("[CTRL] DS18B20 desconectado. Aquecedor OFF (fail-safe).");
        }
    }

    // (C) Upload das MÉDIAS a cada 5 min
    if ((now - lastUploadTemp >= UPLOAD_MS) && nTemp > 0)
    {
        lastUploadTemp = now;
        float media5m = sumTemp / nTemp;
        sumTemp = 0.0;
        nTemp = 0;

        char path[64];
        uint64_t ts = epoch_ms();
        snprintf(path, sizeof(path), "/aquario/temperatura/%llu", (unsigned long long)ts);
        Database.set<float>(aClient, path, media5m, processData, "RTDB_Set_Temp");
        Serial.printf("[ENVIO] Temp média (5 min): %.2f °C → %s\n", media5m, path);
    }

    // (D) pH: amostra ~25 s, envia 5 min
    if (now - lastSamplePH >= SAMPLE_MS)
    {
        lastSamplePH = now;
        const int N = 12;
        long somaADC = 0;
        for (int i = 0; i < N; i++)
        {
            somaADC += analogRead(PH_ADC_PIN);
            delayMicroseconds(100);
        }
        const float adc = somaADC / (float)N;
        const float volts = adc * (ADC_VREF / ADC_MAX_COUNTS);

        if (!phInit)
        {
            emaV = volts;
            phInit = true;
        }
        else
        {
            emaV = ALPHA * volts + (1.0f - ALPHA) * emaV;
        }

        float pH = M_PH * emaV + B_PH;
        if (pH < PH_MIN)
            pH = PH_MIN;
        if (pH > PH_MAX)
            pH = PH_MAX;

        sumPH += pH;
        nPH++;
        gLastPH = pH;

#if LOG_HEARTBEAT
        Serial.printf("[AMOSTRA] pH=%.2f (V=%.3f) | Agua=%s | Cascata=%s\n",
                      pH, emaV,
                      waterOk ? "OK" : "BAIXO",
                      waterfallOn ? "LIGADA" : "DESLIGADA");
#endif
    }

    if ((now - lastUploadPH >= UPLOAD_MS) && nPH > 0)
    {
        lastUploadPH = now;
        float mediaPH5m = sumPH / nPH;
        sumPH = 0.0;
        nPH = 0;

        char path[64];
        uint64_t ts = epoch_ms();
        snprintf(path, sizeof(path), "/aquario/ph/%llu", (unsigned long long)ts);
        Database.set<float>(aClient, path, mediaPH5m, processData, "RTDB_Set_pH");
        Serial.printf("[ENVIO] pH médio (5 min): %.2f → %s\n", mediaPH5m, path);
    }

    // (E) BUZZER
    if (!waterOk)
        buzzerPatternWaterLow();
    else
        buzzerOff();

    // (F) LCD
    handleButton();
    if (now - tLastLcd >= LCD_REFRESH_MS)
    {
        tLastLcd = now;
        updateLCD();
    }
    if (autoRotate && (now - tLastRotate >= ROTATE_EVERY_MS))
    {
        tLastRotate = now;
        currentScreen = (currentScreen == Screen::RESUMO) ? Screen::DETALHE : Screen::RESUMO;
        updateLCD();
    }

    // (G) Serviço do alimentador
    feederRun();

    // (H) Agenda simples: 12h entre alimentações
    {
        static bool firstInit = true;
        const uint64_t nowEpoch = epoch_ms();

        if (firstInit)
        {
            if (lastFeedTs == 0)
            {
                lastFeedTs = (nowEpoch > FEED_INTERVAL_MS ? nowEpoch - FEED_INTERVAL_MS : 0);
            }
            firstInit = false;
        }

        if (!feederBusy && (nowEpoch - lastFeedTs >= FEED_INTERVAL_MS))
        {
            feederRequest(1);
            if (fbReady())
            {
                Database.set<uint64_t>(aClient, "/aquario/feeder/logs/last_ts", nowEpoch, App::processData, "RTDB_Log_Feeder_ts");
            }
            Serial.println("[FEEDER] Alimentacao automatica (agenda 12h) solicitada");
        }
    }
}
