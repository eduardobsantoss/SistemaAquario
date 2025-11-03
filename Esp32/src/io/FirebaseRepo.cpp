#include "io/FirebaseRepo.h"
#include <time.h>

// ==== util epoch ms ====
static uint64_t epoch_ms_safe() {
  time_t s = time(nullptr);
  if (s > 100000) return (uint64_t)s * 1000ULL;
  return (uint64_t)millis();
}
uint64_t FirebaseRepo::epochMillisSafe() { return epoch_ms_safe(); }

// ==== callback padrão ====
void FirebaseRepo::onAsync(AsyncResult& r) {
  if (r.isError()) {
    const int code = r.error().code();
    Serial.printf("[FB-Repo][%s] ERROR %d: %s\n",
                  r.uid().c_str(),
                  code,
                  r.error().message().c_str());
  }
}

// ==== inicialização ====
void FirebaseRepo::begin(const char* apiKey,
                         const char* userEmail,
                         const char* userPass,
                         const char* databaseUrl,
                         bool insecureTLS)
{
  _apiKey = apiKey;
  _email  = userEmail;
  _pass   = userPass;
  _dbUrl  = databaseUrl;

  if (insecureTLS) _ssl.setInsecure();

  if (_auth) { delete _auth; _auth = nullptr; }
  _auth = new UserAuth(_apiKey.c_str(), _email.c_str(), _pass.c_str());

  initializeApp(_client, _app, getAuth(*_auth), FirebaseRepo::onAsync, "authTask");
  _app.getApp<RealtimeDatabase>(_rtdb);
  _rtdb.url(_dbUrl.c_str());

  _readyNotified = false;
  _feederReady   = false;
}

void FirebaseRepo::handle() {
  _app.loop();
  if (_app.ready() && !_readyNotified) {
    _readyNotified = true;
    Serial.println("[FirebaseRepo] App autenticado e pronto.");
  }
  if (_app.ready() && !_feederReady) ensureFeederNodes();
}

bool FirebaseRepo::ready() { return _app.ready(); }

// ==== estados ====
void FirebaseRepo::setHeaterState(bool on) {
  if (!ready()) return;
  _rtdb.set<bool>(_client, "/aquario/relay/heater", on, onAsync, "relay_heater");
}

void FirebaseRepo::setWaterfallState(bool on) {
  if (!ready()) return;
  _rtdb.set<bool>(_client, "/aquario/relay/waterfall", on, onAsync, "relay_waterfall");
}

void FirebaseRepo::setWaterOk(bool ok) {
  if (!ready()) return;
  _rtdb.set<bool>(_client, "/aquario/float/water_ok", ok, onAsync, "water_ok");
}

// ==== instantâneo ====
void FirebaseRepo::setTempCurrent(float v){
  if (!ready()) return;
  _rtdb.set<float>(_client, "/aquario/temperatura_current", v, onAsync, "temp_cur");
}

void FirebaseRepo::setPhCurrent(float v){
  if (!ready()) return;
  _rtdb.set<float>(_client, "/aquario/ph_current", v, onAsync, "ph_cur");
}

// ==== modos / auditoria ====
void FirebaseRepo::setMode(const String& actuator, const String& modeStr) {
  if (!ready()) return;
  String path = "/aquario/controle/" + actuator + "/mode";
  _rtdb.set<String>(_client, path.c_str(), modeStr, onAsync, "mode");
}

void FirebaseRepo::logManualOverride(const String& actuator, bool value, const char* reason) {
  if (!ready()) return;
  char key[24]; snprintf(key, sizeof(key), "%llu", (unsigned long long)epochMillisSafe());
  String base = String("/aquario/controle/") + actuator + "/logs/" + key;

  _rtdb.set<String>(_client, (base + "/origin").c_str(), "manual", onAsync);
  _rtdb.set<bool  >(_client, (base + "/value").c_str(),  value, onAsync);
  _rtdb.set<String>(_client, (base + "/reason").c_str(), String(reason), onAsync);
}

void FirebaseRepo::logHeaterDecision(float tC, bool newState, float onThr, float offThr, const char* reason) {
  if (!ready()) return;
  char key[24]; snprintf(key, sizeof(key), "%llu", (unsigned long long)epochMillisSafe());
  String base = String("/aquario/controle/heater/logs/") + key;

  _rtdb.set<float>(_client, (base + "/temp").c_str(), tC, onAsync, "heater_temp");
  _rtdb.set<bool >(_client, (base + "/state").c_str(), newState, onAsync, "heater_state");
  _rtdb.set<float>(_client, (base + "/on_thr").c_str(), onThr, onAsync, "heater_on");
  _rtdb.set<float>(_client, (base + "/off_thr").c_str(), offThr, onAsync, "heater_off");
  _rtdb.set<String>(_client, (base + "/reason").c_str(), String(reason), onAsync, "heater_reason");
}

// ==== heartbeat ====
void FirebaseRepo::publishLastSeen() {
  if (!ready()) return;
  _rtdb.set<uint64_t>(_client, "/aquario/status/last_seen",
                      epochMillisSafe(), onAsync, "last_seen");
}

// ==== séries ====
void FirebaseRepo::pushTempAvg(float avgC) {
  if (!ready()) return;
  String key = String((uint64_t)epochMillisSafe());
  String path = "/aquario/temperatura/" + key;
  _rtdb.set<float>(_client, path.c_str(), avgC, onAsync, "avg_temp");
}

void FirebaseRepo::pushPHAvg(float avgPH) {
  if (!ready()) return;
  String key = String((uint64_t)epochMillisSafe());
  String path = "/aquario/ph/" + key;
  _rtdb.set<float>(_client, path.c_str(), avgPH, onAsync, "avg_ph");
}

// ==== feeder ====
void FirebaseRepo::ensureFeederNodes() {
  if (!ready() || _feederReady) return;

  // feeder edge
  _rtdb.set<bool>(_client, "/aquario/controle/feeder/feed_now", false, onAsync, "feed_now");

  // comandos edge
  _rtdb.set<bool>(_client, "/aquario/controle/heater/turn_on_now", false, onAsync, "heater_ton");
  _rtdb.set<bool>(_client, "/aquario/controle/waterfall/turn_on_now", false, onAsync, "wf_ton");

  // status feeder
  _rtdb.set<bool>(_client, "/aquario/status/feeder/busy", false, onAsync, "feeder_busy");
  _rtdb.set<uint64_t>(_client, "/aquario/status/feeder/last_ts", 0, onAsync, "feeder_last_ts");

  _feederReady = true;
  Serial.println("[FirebaseRepo] Nós do feeder/commands garantidos.");
}

bool FirebaseRepo::pollFeedNowAndReset() {
  if (!ready()) return false;
  bool want = false;
  want = _rtdb.get<bool>(_client, "/aquario/controle/feeder/feed_now");
  if (want) {
    _rtdb.set<bool>(_client, "/aquario/controle/feeder/feed_now", false, onAsync, "feed_reset");
    return true;
  }
  return false;
}

bool FirebaseRepo::pollHeaterTurnOnNowEdge() {
  if (!ready()) return false;
  bool want = _rtdb.get<bool>(_client, "/aquario/controle/heater/turn_on_now");
  if (want) {
    _rtdb.set<bool>(_client, "/aquario/controle/heater/turn_on_now", false, onAsync, "heater_ton_rst");
    return true;
  }
  return false;
}

bool FirebaseRepo::pollWaterfallTurnOnNowEdge() {
  if (!ready()) return false;
  bool want = _rtdb.get<bool>(_client, "/aquario/controle/waterfall/turn_on_now");
  if (want) {
    _rtdb.set<bool>(_client, "/aquario/controle/waterfall/turn_on_now", false, onAsync, "wf_ton_rst");
    return true;
  }
  return false;
}

void FirebaseRepo::setFeederBusy(bool busy) {
  if (!ready()) return;
  _rtdb.set<bool>(_client, "/aquario/status/feeder/busy", busy, onAsync, "feeder_busy_set");
}

void FirebaseRepo::setFeederLastTs(uint64_t epochMs) {
  if (!ready()) return;
  _rtdb.set<uint64_t>(_client, "/aquario/status/feeder/last_ts", epochMs, onAsync, "feeder_last_ts_set");
}
