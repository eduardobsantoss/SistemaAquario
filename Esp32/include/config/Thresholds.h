#pragma once

// Temperatura alvo/segurança
static constexpr float T_SET      = 26.0f;
static constexpr float T_HYST     = 1.0f;
static constexpr float T_MIN_SAFE = 15.0f;
static constexpr float T_MAX_SAFE = 35.0f;

inline float T_MIN_ON()  { return T_SET - T_HYST; }
inline float T_MAX_OFF() { return T_SET + T_HYST; }

// Anti bate-bate
static constexpr unsigned long MIN_SWITCH_MS = 30000;

// pH
static constexpr float M_PH   = -5.77f;
static constexpr float B_PH   = 22.00f;
static constexpr float PH_MIN = 0.0f;
static constexpr float PH_MAX = 14.0f;

// ADC
static constexpr float ADC_MAX_COUNTS = 4095.0f;
static constexpr float ADC_VREF       = 3.3f;

// Janela e cadência
static constexpr unsigned long SAMPLE_MS = 25000;   // ~25 s
static constexpr unsigned long UPLOAD_MS = 300000;  // 5 min

// Confirmação nível d’água
#define T_LOW_CONFIRM_MS  2000
#define T_HIGH_CONFIRM_MS 1000

// Alimentador
static constexpr int  STEPS_PER_REV             = 4096;
static constexpr int  FEED_STEPS_PER_PORTION    = 4096; // calibrar
static constexpr uint8_t MAX_PORTIONS_PER_EVENT = 2;
static constexpr unsigned long FEED_INTERVAL_MS = 12UL*60UL*60UL*1000UL;
static constexpr unsigned long FEED_STEP_INTERVAL_MS = 2;

// Filtro pH
static constexpr float ALPHA_PH = 0.15f;
