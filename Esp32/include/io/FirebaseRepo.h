#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

/**
 * Módulo FirebaseRepo — compatível com mobizt/FirebaseClient v1.5.11
 * ---------------------------------------------------------------
 *  - Autenticação por e-mail/senha
 *  - RTDB assíncrono (set/get)
 *  - Logs, heartbeat, séries, comandos edge
 */
class FirebaseRepo {
public:
  // ---- ciclo de vida ----
  void begin(const char* apiKey,
             const char* userEmail,
             const char* userPass,
             const char* databaseUrl,
             bool insecureTLS = true);

  void handle();
  bool ready();

  // ---- estados / sensores ----
  void setHeaterState(bool on);
  void setWaterfallState(bool on);
  void setWaterOk(bool ok);

  inline void setHeater(bool on)    { setHeaterState(on); }
  inline void setWaterfall(bool on) { setWaterfallState(on); }

  // instantâneo
  void setTempCurrent(float v);
  void setPhCurrent(float v);

  // ---- modos / auditoria ----
  void setMode(const String& actuator, const String& modeStr);
  void logManualOverride(const String& actuator, bool value, const char* reason);
  void logHeaterDecision(float tC, bool newState, float onThr, float offThr, const char* reason);

  // ---- heartbeat ----
  void publishLastSeen();
  inline void lastSeen() { publishLastSeen(); }

  // ---- séries históricas ----
  void pushTempAvg(float avgC);
  void pushPHAvg(float avgPH);
  inline void pushPhAvg(float v) { pushPHAvg(v); }

  // ---- comandos (edge) ----
  bool pollFeedNowAndReset();
  inline bool pollFeedNowEdge() { return pollFeedNowAndReset(); }

  bool pollHeaterTurnOnNowEdge();
  bool pollWaterfallTurnOnNowEdge();

  // ---- feeder ----
  void ensureFeederNodes();
  inline void ensureFeederTreeOnce(int, int, int) { ensureFeederNodes(); }
  void setFeederBusy(bool busy);
  void setFeederLastTs(uint64_t epochMs);

  const String& dbUrl() const { return _dbUrl; }

private:
  // objetos FirebaseClient v1.5.11
  WiFiClientSecure    _ssl;
  network_config_data _net{};
  AsyncClientClass    _client{_ssl, _net};
  FirebaseApp         _app;
  RealtimeDatabase    _rtdb;
  UserAuth*           _auth{nullptr};

  String _apiKey, _email, _pass, _dbUrl;
  bool   _readyNotified{false};
  bool   _feederReady{false};

  static uint64_t epochMillisSafe();
  static void onAsync(AsyncResult& r);
};
