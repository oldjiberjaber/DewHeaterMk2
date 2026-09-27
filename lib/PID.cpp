#include <Preferences.h>

// --- Safety & Config Settings ---
const float MAX_SAFE_TEMP = 31.0; // Cutoff temperature in Celsius
uint32_t settleTimeSec = 10;
uint32_t testTimeSec = 60;
const uint16_t samples = 60;
const float inputSpan = 30;
const float outputSpan = 255;
float outputStart = 0;
float outputStep = 128;

// --- Active Tunings ---
float H0_Kp = 25.0f;
float H0_Ki = 10.0f;
float H0_Kd = 25.0f;

float H1_Kp = 25.0f;
float H1_Ki = 4.0f;
float H1_Kd = 10.0f;

// --- Soft-Landing Predictive Governor Settings & Diagnostics ---
#ifndef _SOFTLAND_CONFIG_H_
#define _SOFTLAND_CONFIG_H_
struct SoftLandingConfig {
  bool enabled = true;
  float agg0 = 1.0f; // Multiplier for CH0 (0.2 - 3.0)
  float agg1 = 1.0f; // Multiplier for CH1 (0.2 - 3.0)
};
#endif
SoftLandingConfig softLandingConfig;

// --- Adaptive Self-Learning Thermal Engine ---
#ifndef _ADAPTIVE_LEARNING_H_
#define _ADAPTIVE_LEARNING_H_
struct AdaptiveLearningConfig {
  bool enabled = true;
  int adaptationSpeed = 1; // 0: Conservative, 1: Balanced, 2: Fast
  
  // Learned Parameters
  float kloss0 = 12.0f; // PWM units per °C delta above ambient (CH0)
  float kloss1 = 14.0f; // PWM units per °C delta above ambient (CH1)
  float leadSec0 = 20.0f; // Learned predictive lead horizon in seconds (CH0)
  float leadSec1 = 25.0f; // Learned predictive lead horizon in seconds (CH1)
  
  // Stats
  int cycles0 = 0;
  int cycles1 = 0;
  float lastError0 = 0.0f; // Last cycle peak landing error in °C
  float lastError1 = 0.0f; // Last cycle peak landing error in °C
  float confidence0 = 50.0f; // 0-100% convergence confidence
  float confidence1 = 50.0f; // 0-100% convergence confidence
};
#endif
AdaptiveLearningConfig learningConfig;

enum LearningEngineState {
  LEARNING_STATE_STEADY = 0,
  LEARNING_STATE_TRANSITION = 1,
  LEARNING_STATE_STANDBY = 2
};
int engineState0 = LEARNING_STATE_STANDBY;
int engineState1 = LEARNING_STATE_STANDBY;

unsigned long steadyStartTime0 = 0;
unsigned long steadyStartTime1 = 0;
float steadyPwmAccum0 = 0.0f;
float steadyPwmAccum1 = 0.0f;
int steadySampleCount0 = 0;
int steadySampleCount1 = 0;

bool inDescent0 = false;
bool inDescent1 = false;
float descentMinTemp0 = 999.0f;
float descentMinTemp1 = 999.0f;
float descentSetpoint0 = 0.0f;
float descentSetpoint1 = 0.0f;
unsigned long descentStartTime0 = 0;
unsigned long descentStartTime1 = 0;
unsigned long lastLearningNvsSaveMillis = 0;
bool learningModelModified = false;

float ch0_rate = 0.0f; // Live cooling/warming rate in °C/min
float ch1_rate = 0.0f; // Live cooling/warming rate in °C/min
bool ch0_landing_active = false;
bool ch1_landing_active = false;

unsigned long lastRateCalcMillis0 = 0;
unsigned long lastRateCalcMillis1 = 0;
float lastTemp0 = 0.0f;
float lastTemp1 = 0.0f;
bool rateInitialized0 = false;
bool rateInitialized1 = false;

extern String TempC;

// --- Global Variables ---
float Input0 = 0, Output0 = 0, Setpoint0 = 10, Kp = 0, Ki = 0, Kd = 0;
float Input1 = 0, Output1 = 0, Setpoint1 = 10;

bool heaterEnabled0 = true;
bool heaterEnabled1 = true;

void SetHeaterValues();

bool tuningRequired0 = false;
bool tuningRequired1 = false;

// --- Tuning State Machine ---
enum TunerState {
  TUNER_IDLE = 0,
  TUNER_SETTLING = 1,
  TUNER_TESTING = 2,
  TUNER_COMPLETE = 3,
  TUNER_ABORTED = 4,
  TUNER_ERROR = 5
};

struct TuningStatus {
  TunerState state = TUNER_IDLE;
  int heater = -1; // 0 for CH0, 1 for CH1, -1 for none
  float currentTemp = 0.0f;
  float currentOutput = 0.0f;
  unsigned long startMillis = 0;
  uint32_t settleTime = 10;
  uint32_t testTime = 60;
  float stepPWM = 128.0f;
  int tuningRule = 0; // 0: ZN_PID, 1: NoOvershoot_PID, 2: CohenCoon_PID, 3: DampedOsc_PID
  
  // Snapshots
  float orig_Kp = 0;
  float orig_Ki = 0;
  float orig_Kd = 0;
  
  // Calculated Results
  float calc_Kp = 0;
  float calc_Ki = 0;
  float calc_Kd = 0;
  float processGain = 0;
  float deadTime = 0;
  float tau = 0;
  String message = "Tuner Ready";
};

TuningStatus tuningStatus;

// --- Objects ---
sTune tuner0 = sTune(&Input0, &Output0, tuner0.ZN_PID, tuner0.directIP, tuner0.printOFF);
sTune tuner1 = sTune(&Input1, &Output1, tuner1.ZN_PID, tuner1.directIP, tuner1.printOFF);
QuickPID heater0(&Input0, &Output0, &Setpoint0);
QuickPID heater1(&Input1, &Output1, &Setpoint1);

// --- NVS Storage for PID Tunings ---
void loadPIDFromNVS() {
  Preferences prefs;
  if (prefs.begin("pid", true)) {
    H0_Kp = prefs.getFloat("h0_kp", 25.0f);
    H0_Ki = prefs.getFloat("h0_ki", 10.0f);
    H0_Kd = prefs.getFloat("h0_kd", 25.0f);
    H1_Kp = prefs.getFloat("h1_kp", 25.0f);
    H1_Ki = prefs.getFloat("h1_ki", 4.0f);
    H1_Kd = prefs.getFloat("h1_kd", 10.0f);
    prefs.end();
  }
  if (isnan(H0_Kp) || isinf(H0_Kp) || H0_Kp <= 0) H0_Kp = 25.0f;
  if (isnan(H0_Ki) || isinf(H0_Ki) || H0_Ki < 0) H0_Ki = 10.0f;
  if (isnan(H0_Kd) || isinf(H0_Kd) || H0_Kd < 0) H0_Kd = 25.0f;
  if (isnan(H1_Kp) || isinf(H1_Kp) || H1_Kp <= 0) H1_Kp = 25.0f;
  if (isnan(H1_Ki) || isinf(H1_Ki) || H1_Ki < 0) H1_Ki = 4.0f;
  if (isnan(H1_Kd) || isinf(H1_Kd) || H1_Kd < 0) H1_Kd = 10.0f;

  Serial.printf("Loaded PID from NVS: H0(%.2f, %.3f, %.2f) | H1(%.2f, %.3f, %.2f)\n", 
                H0_Kp, H0_Ki, H0_Kd, H1_Kp, H1_Ki, H1_Kd);
}

void savePIDToNVS(int heater, float kp, float ki, float kd) {
  if (isnan(kp) || isinf(kp) || kp <= 0) kp = 25.0f;
  if (isnan(ki) || isinf(ki) || ki < 0) ki = (heater == 0 ? 10.0f : 4.0f);
  if (isnan(kd) || isinf(kd) || kd < 0) kd = (heater == 0 ? 25.0f : 10.0f);

  Preferences prefs;
  if (prefs.begin("pid", false)) {
    if (heater == 0) {
      prefs.putFloat("h0_kp", kp);
      prefs.putFloat("h0_ki", ki);
      prefs.putFloat("h0_kd", kd);
    } else if (heater == 1) {
      prefs.putFloat("h1_kp", kp);
      prefs.putFloat("h1_ki", ki);
      prefs.putFloat("h1_kd", kd);
    }
    prefs.end();
    Serial.printf("Saved PID to NVS for CH%d: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", heater, kp, ki, kd);
  }
}

// --- NVS Storage for Default Setpoint Offsets ---
void loadOffsetsFromNVS() {
  Preferences prefs;
  if (prefs.begin("offsets", true)) {
    DP_Offset0 = prefs.getFloat("offset0", 4.0f);
    DP_Offset1 = prefs.getFloat("offset1", 4.0f);
    prefs.end();
  }
  if (isnan(DP_Offset0) || isinf(DP_Offset0) || DP_Offset0 < 0.0f || DP_Offset0 > 30.0f) DP_Offset0 = 4.0f;
  if (isnan(DP_Offset1) || isinf(DP_Offset1) || DP_Offset1 < 0.0f || DP_Offset1 > 30.0f) DP_Offset1 = 4.0f;

  Serial.printf("Loaded Setpoint Offsets from NVS: H0=%.2fC | H1=%.2fC\n", DP_Offset0, DP_Offset1);
}

void saveOffsetsToNVS(float off0, float off1) {
  if (isnan(off0) || isinf(off0) || off0 < 0.0f || off0 > 30.0f) off0 = 4.0f;
  if (isnan(off1) || isinf(off1) || off1 < 0.0f || off1 > 30.0f) off1 = 4.0f;

  DP_Offset0 = off0;
  DP_Offset1 = off1;

  Preferences prefs;
  if (prefs.begin("offsets", false)) {
    prefs.putFloat("offset0", off0);
    prefs.putFloat("offset1", off1);
    prefs.end();
    Serial.printf("Saved Default Setpoint Offsets to NVS: H0=%.2fC | H1=%.2fC\n", off0, off1);
  }

  // Recalculate setpoint targets and compute
  SetHeaterValues();
  if (tuningStatus.state == TUNER_IDLE) {
    if (heaterEnabled0) {
      heater0.Compute();
      ledcWrite(0, (int)Output0);
      pwmDuty[0] = returnPWR((int)Output0);
    }
    if (heaterEnabled1) {
      heater1.Compute();
      ledcWrite(1, (int)Output1);
      pwmDuty[1] = returnPWR((int)Output1);
    }
  }
}

// --- NVS Storage for Soft-Landing Predictive Governor ---
void loadSoftLandingFromNVS() {
  Preferences prefs;
  if (prefs.begin("softland", true)) {
    softLandingConfig.enabled = prefs.getBool("enabled", true);
    softLandingConfig.agg0 = prefs.getFloat("agg0", 1.0f);
    softLandingConfig.agg1 = prefs.getFloat("agg1", 1.0f);
    prefs.end();
  }
  if (isnan(softLandingConfig.agg0) || isinf(softLandingConfig.agg0) || softLandingConfig.agg0 < 0.1f || softLandingConfig.agg0 > 5.0f) {
    softLandingConfig.agg0 = 1.0f;
  }
  if (isnan(softLandingConfig.agg1) || isinf(softLandingConfig.agg1) || softLandingConfig.agg1 < 0.1f || softLandingConfig.agg1 > 5.0f) {
    softLandingConfig.agg1 = 1.0f;
  }
  Serial.printf("Loaded Soft-Landing from NVS: Enabled=%s, Agg0=%.2f, Agg1=%.2f\n",
                softLandingConfig.enabled ? "true" : "false", softLandingConfig.agg0, softLandingConfig.agg1);
}

void saveSoftLandingToNVS(bool enabled, float agg0, float agg1) {
  softLandingConfig.enabled = enabled;
  softLandingConfig.agg0 = constrain(agg0, 0.2f, 3.0f);
  softLandingConfig.agg1 = constrain(agg1, 0.2f, 3.0f);

  Preferences prefs;
  if (prefs.begin("softland", false)) {
    prefs.putBool("enabled", softLandingConfig.enabled);
    prefs.putFloat("agg0", softLandingConfig.agg0);
    prefs.putFloat("agg1", softLandingConfig.agg1);
    prefs.end();
    Serial.printf("Saved Soft-Landing to NVS: Enabled=%s, Agg0=%.2f, Agg1=%.2f\n",
                  softLandingConfig.enabled ? "true" : "false", softLandingConfig.agg0, softLandingConfig.agg1);
  }
}

// --- NVS Storage for Adaptive Self-Learning Thermal Engine ---
void loadLearningFromNVS() {
  Preferences prefs;
  if (prefs.begin("learning", true)) {
    learningConfig.enabled = prefs.getBool("enabled", true);
    learningConfig.adaptationSpeed = prefs.getInt("speed", 1);
    learningConfig.kloss0 = prefs.getFloat("kloss0", 12.0f);
    learningConfig.kloss1 = prefs.getFloat("kloss1", 14.0f);
    learningConfig.leadSec0 = prefs.getFloat("lead0", 20.0f);
    learningConfig.leadSec1 = prefs.getFloat("lead1", 25.0f);
    learningConfig.cycles0 = prefs.getInt("cycles0", 0);
    learningConfig.cycles1 = prefs.getInt("cycles1", 0);
    learningConfig.lastError0 = prefs.getFloat("lasterr0", 0.0f);
    learningConfig.lastError1 = prefs.getFloat("lasterr1", 0.0f);
    learningConfig.confidence0 = prefs.getFloat("conf0", 50.0f);
    learningConfig.confidence1 = prefs.getFloat("conf1", 50.0f);
    prefs.end();
  }
  if (isnan(learningConfig.kloss0) || learningConfig.kloss0 < 2.0f || learningConfig.kloss0 > 50.0f) learningConfig.kloss0 = 12.0f;
  if (isnan(learningConfig.kloss1) || learningConfig.kloss1 < 2.0f || learningConfig.kloss1 > 50.0f) learningConfig.kloss1 = 14.0f;
  if (isnan(learningConfig.leadSec0) || learningConfig.leadSec0 < 5.0f || learningConfig.leadSec0 > 120.0f) learningConfig.leadSec0 = 20.0f;
  if (isnan(learningConfig.leadSec1) || learningConfig.leadSec1 < 5.0f || learningConfig.leadSec1 > 120.0f) learningConfig.leadSec1 = 25.0f;
  if (learningConfig.adaptationSpeed < 0 || learningConfig.adaptationSpeed > 2) learningConfig.adaptationSpeed = 1;

  Serial.printf("Loaded Adaptive Learning from NVS: Enabled=%s | CH0(Kloss=%.1f, Lead=%.1fs, Cycles=%d) | CH1(Kloss=%.1f, Lead=%.1fs, Cycles=%d)\n",
                learningConfig.enabled ? "true" : "false",
                learningConfig.kloss0, learningConfig.leadSec0, learningConfig.cycles0,
                learningConfig.kloss1, learningConfig.leadSec1, learningConfig.cycles1);
}

void saveLearningToNVS() {
  Preferences prefs;
  if (prefs.begin("learning", false)) {
    prefs.putBool("enabled", learningConfig.enabled);
    prefs.putInt("speed", learningConfig.adaptationSpeed);
    prefs.putFloat("kloss0", learningConfig.kloss0);
    prefs.putFloat("kloss1", learningConfig.kloss1);
    prefs.putFloat("lead0", learningConfig.leadSec0);
    prefs.putFloat("lead1", learningConfig.leadSec1);
    prefs.putInt("cycles0", learningConfig.cycles0);
    prefs.putInt("cycles1", learningConfig.cycles1);
    prefs.putFloat("lasterr0", learningConfig.lastError0);
    prefs.putFloat("lasterr1", learningConfig.lastError1);
    prefs.putFloat("conf0", learningConfig.confidence0);
    prefs.putFloat("conf1", learningConfig.confidence1);
    prefs.end();
    learningModelModified = false;
    Serial.println(">>> Saved Adaptive Learning Parameters to NVS Flash");
  }
}

void resetLearningToDefaults() {
  learningConfig.kloss0 = 12.0f;
  learningConfig.kloss1 = 14.0f;
  learningConfig.leadSec0 = 20.0f;
  learningConfig.leadSec1 = 25.0f;
  learningConfig.cycles0 = 0;
  learningConfig.cycles1 = 0;
  learningConfig.lastError0 = 0.0f;
  learningConfig.lastError1 = 0.0f;
  learningConfig.confidence0 = 50.0f;
  learningConfig.confidence1 = 50.0f;
  saveLearningToNVS();
  Serial.println(">>> Reset Adaptive Learning Model to Factory Defaults");
}

// --- PID Init ---
void initPID()
{
  loadPIDFromNVS();
  loadOffsetsFromNVS();
  loadSoftLandingFromNVS();
  loadLearningFromNVS();

  heater0.SetTunings(H0_Kp, H0_Ki, H0_Kd);
  heater1.SetTunings(H1_Kp, H1_Ki, H1_Kd);

  heater0.SetOutputLimits(0, outputSpan);
  heater0.SetSampleTimeUs(200000);
  heater0.SetProportionalMode(heater0.pMode::pOnError);
  heater0.SetAntiWindupMode(heater0.iAwMode::iAwClamp);
  heater0.SetMode(QuickPID::Control::automatic);

  heater1.SetOutputLimits(0, outputSpan);
  heater1.SetSampleTimeUs(200000);
  heater1.SetProportionalMode(heater1.pMode::pOnError);
  heater1.SetAntiWindupMode(heater1.iAwMode::iAwClamp);
  heater1.SetMode(QuickPID::Control::automatic);

  tuner0.Configure(inputSpan, outputSpan, outputStart, outputStep, testTimeSec, settleTimeSec, samples);
  tuner0.SetEmergencyStop(MAX_SAFE_TEMP);
  tuner1.Configure(inputSpan, outputSpan, outputStart, outputStep, testTimeSec, settleTimeSec, samples);
  tuner1.SetEmergencyStop(MAX_SAFE_TEMP);
}

// --- Auto-Tuning Control API ---
bool startTuning(int heater, float stepPWM, uint32_t testSec, uint32_t settleSec, int rule) {
  if (heater < 0 || heater > 1) return false;
  if (!DT[heater].Present) {
    tuningStatus.state = TUNER_ERROR;
    tuningStatus.message = "Selected sensor is not present/connected!";
    return false;
  }

  // Choose tuning rule
  sTune::TuningMethod method = sTune::ZN_PID;
  if (rule == 1) method = sTune::NoOvershoot_PID;
  else if (rule == 2) method = sTune::CohenCoon_PID;
  else if (rule == 3) method = sTune::DampedOsc_PID;

  stepPWM = constrain(stepPWM, 10, 255);
  testSec = constrain(testSec, 20, 600);
  settleSec = constrain(settleSec, 5, 120);
  uint16_t sampleCount = testSec; // 1 sample per second aligned with 1Hz DS18B20 polling

  // Force both heaters OFF immediately to prepare clean baseline
  Output0 = 0;
  Output1 = 0;
  ledcWrite(0, 0);
  ledcWrite(1, 0);

  // Put normal PIDs into manual mode during tuning to prevent interference
  heater0.SetMode(QuickPID::Control::manual);
  heater1.SetMode(QuickPID::Control::manual);

  tuningStatus.heater = heater;
  tuningStatus.settleTime = settleSec;
  tuningStatus.testTime = testSec;
  tuningStatus.stepPWM = stepPWM;
  tuningStatus.tuningRule = rule;
  tuningStatus.startMillis = millis();
  tuningStatus.state = TUNER_SETTLING;
  tuningStatus.calc_Kp = 0;
  tuningStatus.calc_Ki = 0;
  tuningStatus.calc_Kd = 0;
  tuningStatus.processGain = 0;
  tuningStatus.deadTime = 0;
  tuningStatus.tau = 0;
  tuningStatus.message = "Baseline settling in progress. Non-tuned channels forced OFF.";

  if (heater == 0) {
    tuningStatus.orig_Kp = H0_Kp;
    tuningStatus.orig_Ki = H0_Ki;
    tuningStatus.orig_Kd = H0_Kd;
    tuner0.SetControllerAction(sTune::directIP);
    tuner0.SetTuningMethod(method);
    tuner0.Configure(inputSpan, outputSpan, 0, stepPWM, testSec, settleSec, sampleCount);
    tuner0.SetEmergencyStop(MAX_SAFE_TEMP);
    tuner0.Reset();
    tuningRequired0 = true;
    tuningRequired1 = false;
  } else {
    tuningStatus.orig_Kp = H1_Kp;
    tuningStatus.orig_Ki = H1_Ki;
    tuningStatus.orig_Kd = H1_Kd;
    tuner1.SetControllerAction(sTune::directIP);
    tuner1.SetTuningMethod(method);
    tuner1.Configure(inputSpan, outputSpan, 0, stepPWM, testSec, settleSec, sampleCount);
    tuner1.SetEmergencyStop(MAX_SAFE_TEMP);
    tuner1.Reset();
    tuningRequired1 = true;
    tuningRequired0 = false;
  }

  Serial.printf(">>> Auto-Tuner Started for CH%d | Step: %.0f (%.1f%%) | Test: %us | Settle: %us\n",
                heater, stepPWM, (stepPWM / 255.0f) * 100.0f, testSec, settleSec);
  return true;
}

void setHeaterState(int heater, bool enabled) {
  if (heater == 0 || heater == 2) {
    heaterEnabled0 = enabled;
    if (!enabled) {
      Output0 = 0;
      ledcWrite(0, 0);
      heater0.SetMode(QuickPID::Control::manual);
    } else if (tuningStatus.state == TUNER_IDLE) {
      heater0.SetMode(QuickPID::Control::automatic);
      if (DT[0].Present) {
        Input0 = DT[0].Temperature;
        SetHeaterValues();
        heater0.Compute();
        ledcWrite(0, (int)Output0);
      }
    }
  }
  if (heater == 1 || heater == 2) {
    heaterEnabled1 = enabled;
    if (!enabled) {
      Output1 = 0;
      ledcWrite(1, 0);
      heater1.SetMode(QuickPID::Control::manual);
    } else if (tuningStatus.state == TUNER_IDLE) {
      heater1.SetMode(QuickPID::Control::automatic);
      if (DT[1].Present) {
        Input1 = DT[1].Temperature;
        SetHeaterValues();
        heater1.Compute();
        ledcWrite(1, (int)Output1);
      }
    }
  }
  Serial.printf(">>> Heater State Changed: CH0=%s (PWR0=%.0f%%), CH1=%s (PWR1=%.0f%%)\n", 
                heaterEnabled0 ? "ON" : "OFF (Cooldown)", (Output0/255.0f)*100.0f,
                heaterEnabled1 ? "ON" : "OFF (Cooldown)", (Output1/255.0f)*100.0f);
}

void abortTuning() {
  if (tuningStatus.state == TUNER_SETTLING || tuningStatus.state == TUNER_TESTING || tuningStatus.state == TUNER_COMPLETE) {
    tuningStatus.state = TUNER_IDLE;
    tuningStatus.message = "Tuning aborted. Normal operation resumed.";
    tuningRequired0 = false;
    tuningRequired1 = false;
    Output0 = 0;
    Output1 = 0;
    ledcWrite(0, 0);
    ledcWrite(1, 0);

    // Restore original tunings
    heater0.SetTunings(H0_Kp, H0_Ki, H0_Kd);
    heater1.SetTunings(H1_Kp, H1_Ki, H1_Kd);

    // Re-engage normal PID automatic mode smoothly for enabled channels
    if (heaterEnabled0) {
      heater0.SetMode(QuickPID::Control::automatic);
      if (DT[0].Present) {
        Input0 = DT[0].Temperature;
        heater0.Compute();
        ledcWrite(0, (int)Output0);
      }
    }
    if (heaterEnabled1) {
      heater1.SetMode(QuickPID::Control::automatic);
      if (DT[1].Present) {
        Input1 = DT[1].Temperature;
        heater1.Compute();
        ledcWrite(1, (int)Output1);
      }
    }

    Serial.println(">>> Auto-Tuner Aborted. Reverted to normal PID operation.");
  }
}

void applyTuning(int heater, bool saveToNVS) {
  if (heater == 0) {
    if (tuningStatus.calc_Kp > 0 && !isnan(tuningStatus.calc_Kp) && !isinf(tuningStatus.calc_Kp)) {
      H0_Kp = tuningStatus.calc_Kp;
      H0_Ki = tuningStatus.calc_Ki;
      H0_Kd = tuningStatus.calc_Kd;
    }
    heater0.SetTunings(H0_Kp, H0_Ki, H0_Kd);
    if (saveToNVS) savePIDToNVS(0, H0_Kp, H0_Ki, H0_Kd);
  } else if (heater == 1) {
    if (tuningStatus.calc_Kp > 0 && !isnan(tuningStatus.calc_Kp) && !isinf(tuningStatus.calc_Kp)) {
      H1_Kp = tuningStatus.calc_Kp;
      H1_Ki = tuningStatus.calc_Ki;
      H1_Kd = tuningStatus.calc_Kd;
    }
    heater1.SetTunings(H1_Kp, H1_Ki, H1_Kd);
    if (saveToNVS) savePIDToNVS(1, H1_Kp, H1_Ki, H1_Kd);
  }

  // Clear tuner flags
  tuningRequired0 = false;
  tuningRequired1 = false;
  tuningStatus.state = TUNER_IDLE;
  tuningStatus.message = saveToNVS ? "New tunings saved to Flash & active on dew heater!" : "New tunings active on dew heater (RAM only)!";

  // Re-enable normal automatic PID mode for enabled channels
  if (heaterEnabled0) {
    heater0.SetMode(QuickPID::Control::automatic);
    if (DT[0].Present) {
      Input0 = DT[0].Temperature;
      heater0.Compute();
      ledcWrite(0, (int)Output0);
    }
  }
  if (heaterEnabled1) {
    heater1.SetMode(QuickPID::Control::automatic);
    if (DT[1].Present) {
      Input1 = DT[1].Temperature;
      heater1.Compute();
      ledcWrite(1, (int)Output1);
    }
  }

  Serial.printf(">>> Tunings Applied to CH%d (Saved=%s) -> Kp=%.3f, Ki=%.3f, Kd=%.3f. Reverted to normal operation.\n", 
                heater, saveToNVS ? "YES" : "NO",
                (heater == 0 ? H0_Kp : H1_Kp), (heater == 0 ? H0_Ki : H1_Ki), (heater == 0 ? H0_Kd : H1_Kd));
}

void discardTuning(int heater) {
  // Re-confirm original tunings
  heater0.SetTunings(H0_Kp, H0_Ki, H0_Kd);
  heater1.SetTunings(H1_Kp, H1_Ki, H1_Kd);

  // Clear tuner flags
  tuningRequired0 = false;
  tuningRequired1 = false;
  tuningStatus.state = TUNER_IDLE;
  tuningStatus.message = "Calculated tunings discarded. Original settings active. Normal operation resumed.";

  // Re-enable normal automatic PID mode for enabled channels
  if (heaterEnabled0) {
    heater0.SetMode(QuickPID::Control::automatic);
    if (DT[0].Present) {
      Input0 = DT[0].Temperature;
      heater0.Compute();
      ledcWrite(0, (int)Output0);
    }
  }
  if (heaterEnabled1) {
    heater1.SetMode(QuickPID::Control::automatic);
    if (DT[1].Present) {
      Input1 = DT[1].Temperature;
      heater1.Compute();
      ledcWrite(1, (int)Output1);
    }
  }

  Serial.println(">>> Tunings Discarded. Reverted to normal operation with original settings.");
}

void SetHeaterValues()
{
  float currentDP = 15.0f; // Baseline dew point if weather station is offline / syncing
  if (WeeWX_Valid && WeeWX.containsKey("dp")) {
    currentDP = WeeWX["dp"].as<float>();
  } else if (DP.length() > 0 && DP.toFloat() > -50.0f && DP.toFloat() < 60.0f) {
    currentDP = DP.toFloat();
  }

  if (DT[0].Present){ 
    Input0 = DT[0].Temperature; 
    Setpoint0 = currentDP + DP_Offset0;
  } else {
    Output0 = 0;
  }

  if (DT[1].Present){ 
    Input1 = DT[1].Temperature; 
    Setpoint1 = currentDP + DP_Offset1;
  } else {
    Output1 = 0;
  }    
}

void computeDewParallelTunings(sTune &tuner, int rule, float &outKp, float &outKi, float &outKd) {
  float raw_Kp = 0, raw_Ki = 0, raw_Kd = 0;
  tuner.GetAutoTunings(&raw_Kp, &raw_Ki, &raw_Kd);

  float deadTime = tuner.GetDeadTime();
  if (deadTime <= 0 || isnan(deadTime)) deadTime = 10.0f;

  // 1. Convert ISA/Standard Ki (1/Ti) and Kd (1/Td) to QuickPID Parallel form:
  float pKp = (raw_Kp > 0) ? raw_Kp : 20.0f;
  float pKi = pKp * raw_Ki;
  float pKd = (raw_Kd > 0.0001f) ? (pKp / raw_Kd) : (pKp * deadTime * 0.5f);

  // 2. Scale for outdoor thermal dynamics (convective air & radiative heat loss):
  float agg = 2.0f; // Assertive dew prevention
  if (rule == 1) agg = 1.3f;      // NoOvershoot
  else if (rule == 2) agg = 2.2f; // Cohen-Coon
  else if (rule == 3) agg = 1.6f; // Damped Oscillations

  outKp = constrain(pKp * agg, 15.0f, 60.0f);
  outKi = constrain(pKi * agg * 10.0f, 2.0f, 25.0f);
  outKd = constrain(pKd * (agg * 0.8f), 2.0f, 35.0f);
}

float computeSoftLandingOutput(int channel, float currentTemp, float setpoint, float kp, float kd, float ambientTemp, float pidOutput) {
  if (!softLandingConfig.enabled) {
    if (channel == 0) ch0_landing_active = false;
    else ch1_landing_active = false;
    return pidOutput;
  }

  float rate = (channel == 0) ? ch0_rate : ch1_rate;
  float agg = (channel == 0) ? softLandingConfig.agg0 : softLandingConfig.agg1;
  float deltaToTarget = currentTemp - setpoint; // Positive when above target setpoint

  // Active when temperature is above target and falling towards it (cooling rate < -0.05 °C/min)
  if (deltaToTarget > 0.02f && rate < -0.05f) {
    // Predictive lead time estimation based on system thermal lag and learned model
    float learnedLead = (channel == 0) ? learningConfig.leadSec0 : learningConfig.leadSec1;
    float baseLeadSec = (learningConfig.enabled && learnedLead > 5.0f) ? learnedLead : 20.0f;
    if (!learningConfig.enabled && kp > 0.1f && kd > 0.1f) {
      baseLeadSec = (kd / kp) * 15.0f;
    }
    float leadSec = constrain(baseLeadSec * agg, 8.0f, 90.0f);

    // Predict temperature in leadSec seconds
    float predictedTemp = currentTemp + (rate / 60.0f) * leadSec;

    // Trigger soft-landing when projected trajectory reaches capture window (+0.5°C)
    if (predictedTemp <= (setpoint + 0.5f)) {
      // 1. Estimated holding power to balance ambient heat loss at target:
      float ambDelta = max(0.0f, setpoint - ambientTemp);
      float learnedKloss = (channel == 0) ? learningConfig.kloss0 : learningConfig.kloss1;
      float kloss = (learningConfig.enabled && learnedKloss > 2.0f) ? learnedKloss : 12.0f;
      float holdPwm = constrain(ambDelta * kloss * agg, 0.0f, 160.0f);

      // 2. Dynamic descent braking cushion: scales with rate of descent and proximity to target
      float proximityWeight = 1.0f / max(0.10f, deltaToTarget);
      float cushionPwm = constrain((-rate) * 6.0f * agg * proximityWeight, 0.0f, 220.0f);

      float landingPwm = constrain(holdPwm + cushionPwm, 0.0f, 255.0f);

      if (channel == 0) ch0_landing_active = true;
      else ch1_landing_active = true;

      return max(pidOutput, landingPwm);
    }
  }

  if (channel == 0) ch0_landing_active = false;
  else ch1_landing_active = false;

  return pidOutput;
}

// --- Adaptive Self-Learning Thermal Model Updater ---
void updateAdaptiveModel(int channel, float currentTemp, float setpoint, float rate, float ambientTemp, float pwmOutput) {
  if (!learningConfig.enabled) {
    if (channel == 0) engineState0 = LEARNING_STATE_STANDBY;
    else engineState1 = LEARNING_STATE_STANDBY;
    return;
  }

  // Learning rate alpha based on user adaptation speed setting
  float alpha = 0.05f; // Balanced (1)
  if (learningConfig.adaptationSpeed == 0) alpha = 0.02f; // Conservative (0)
  else if (learningConfig.adaptationSpeed == 2) alpha = 0.10f; // Fast (2)

  float deltaTarget = currentTemp - setpoint;
  float ambDelta = setpoint - ambientTemp;

  // --- 1. STEADY-STATE LEARNING: THERMAL CONDUCTANCE (Kloss) ---
  if (fabs(deltaTarget) <= 0.35f && fabs(rate) <= 0.08f && pwmOutput > 5.0f && ambDelta >= 2.0f) {
    if (channel == 0) {
      engineState0 = LEARNING_STATE_STEADY;
      steadyPwmAccum0 += pwmOutput;
      steadySampleCount0++;
      if (steadySampleCount0 >= 15) { // ~3 seconds of steady tracking
        float avgPwm = steadyPwmAccum0 / (float)steadySampleCount0;
        float measuredKloss = avgPwm / ambDelta;
        if (measuredKloss >= 2.0f && measuredKloss <= 45.0f) {
          learningConfig.kloss0 = (1.0f - alpha) * learningConfig.kloss0 + (alpha * measuredKloss);
          learningConfig.confidence0 = min(98.0f, learningConfig.confidence0 + (0.5f * alpha * 10.0f));
          learningModelModified = true;
        }
        steadyPwmAccum0 = 0.0f;
        steadySampleCount0 = 0;
      }
    } else {
      engineState1 = LEARNING_STATE_STEADY;
      steadyPwmAccum1 += pwmOutput;
      steadySampleCount1++;
      if (steadySampleCount1 >= 15) {
        float avgPwm = steadyPwmAccum1 / (float)steadySampleCount1;
        float measuredKloss = avgPwm / ambDelta;
        if (measuredKloss >= 2.0f && measuredKloss <= 45.0f) {
          learningConfig.kloss1 = (1.0f - alpha) * learningConfig.kloss1 + (alpha * measuredKloss);
          learningConfig.confidence1 = min(98.0f, learningConfig.confidence1 + (0.5f * alpha * 10.0f));
          learningModelModified = true;
        }
        steadyPwmAccum1 = 0.0f;
        steadySampleCount1 = 0;
      }
    }
  } else {
    if (channel == 0) {
      steadyPwmAccum0 = 0.0f;
      steadySampleCount0 = 0;
    } else {
      steadyPwmAccum1 = 0.0f;
      steadySampleCount1 = 0;
    }
  }

  // --- 2. TRANSITION LEARNING: DEAD-TIME / LEAD HORIZON via ILC ---
  if (channel == 0) {
    if (deltaTarget > 0.40f && rate < -0.10f) {
      if (!inDescent0) {
        inDescent0 = true;
        descentMinTemp0 = currentTemp;
        descentSetpoint0 = setpoint;
        descentStartTime0 = millis();
      }
      engineState0 = LEARNING_STATE_TRANSITION;
    }

    if (inDescent0) {
      if (currentTemp < descentMinTemp0) descentMinTemp0 = currentTemp;
      unsigned long descentDuration = millis() - descentStartTime0;
      bool settled = (fabs(rate) <= 0.04f && currentTemp <= descentSetpoint0 + 0.35f);
      bool timedOut = (descentDuration > 120000); // 2 minutes max

      if ((settled && descentDuration > 5000) || timedOut) {
        float error = descentSetpoint0 - descentMinTemp0; // Positive = undershot target
        if (error > 0.05f) {
          // Undershot: predictive braking was too late -> increase leadSec
          float leadAdj = error * 10.0f * alpha * 10.0f;
          learningConfig.leadSec0 = constrain(learningConfig.leadSec0 + leadAdj, 8.0f, 60.0f);
        } else if (error < -0.20f) {
          // Delayed descent: braking too early -> reduce leadSec
          float leadAdj = error * 5.0f * alpha * 10.0f;
          learningConfig.leadSec0 = constrain(learningConfig.leadSec0 + leadAdj, 8.0f, 60.0f);
        }
        learningConfig.cycles0++;
        learningConfig.lastError0 = error;
        if (fabs(error) <= 0.25f) {
          learningConfig.confidence0 = min(99.0f, learningConfig.confidence0 + 2.0f);
        }
        learningModelModified = true;
        inDescent0 = false;
        engineState0 = LEARNING_STATE_STANDBY;
      }
    } else if (engineState0 != LEARNING_STATE_STEADY) {
      engineState0 = LEARNING_STATE_STANDBY;
    }
  } else {
    if (deltaTarget > 0.40f && rate < -0.10f) {
      if (!inDescent1) {
        inDescent1 = true;
        descentMinTemp1 = currentTemp;
        descentSetpoint1 = setpoint;
        descentStartTime1 = millis();
      }
      engineState1 = LEARNING_STATE_TRANSITION;
    }

    if (inDescent1) {
      if (currentTemp < descentMinTemp1) descentMinTemp1 = currentTemp;
      unsigned long descentDuration = millis() - descentStartTime1;
      bool settled = (fabs(rate) <= 0.04f && currentTemp <= descentSetpoint1 + 0.35f);
      bool timedOut = (descentDuration > 120000);

      if ((settled && descentDuration > 5000) || timedOut) {
        float error = descentSetpoint1 - descentMinTemp1;
        if (error > 0.05f) {
          float leadAdj = error * 10.0f * alpha * 10.0f;
          learningConfig.leadSec1 = constrain(learningConfig.leadSec1 + leadAdj, 8.0f, 60.0f);
        } else if (error < -0.20f) {
          float leadAdj = error * 5.0f * alpha * 10.0f;
          learningConfig.leadSec1 = constrain(learningConfig.leadSec1 + leadAdj, 8.0f, 60.0f);
        }
        learningConfig.cycles1++;
        learningConfig.lastError1 = error;
        if (fabs(error) <= 0.25f) {
          learningConfig.confidence1 = min(99.0f, learningConfig.confidence1 + 2.0f);
        }
        learningModelModified = true;
        inDescent1 = false;
        engineState1 = LEARNING_STATE_STANDBY;
      }
    } else if (engineState1 != LEARNING_STATE_STEADY) {
      engineState1 = LEARNING_STATE_STANDBY;
    }
  }

  // Auto-save modified learning parameters to NVS flash periodically
  if (learningModelModified && (millis() - lastLearningNvsSaveMillis > 60000)) {
    lastLearningNvsSaveMillis = millis();
    saveLearningToNVS();
  }
}

// --- PID Magic Cycle ---
void PIDMagic()
{
  // Track rolling derivative / cooling rate (°C/min) for each channel
  unsigned long nowMillis = millis();
  if (DT[0].Present) {
    if (!rateInitialized0) {
      lastTemp0 = DT[0].Temperature;
      lastRateCalcMillis0 = nowMillis;
      rateInitialized0 = true;
      ch0_rate = 0.0f;
    } else {
      float dtSec = (nowMillis - lastRateCalcMillis0) / 1000.0f;
      if (dtSec >= 1.0f) {
        float rawRate = ((DT[0].Temperature - lastTemp0) / dtSec) * 60.0f; // °C/min
        ch0_rate = (ch0_rate * 0.70f) + (rawRate * 0.30f);
        lastTemp0 = DT[0].Temperature;
        lastRateCalcMillis0 = nowMillis;
      }
    }
  } else {
    ch0_rate = 0.0f;
    ch0_landing_active = false;
  }

  if (DT[1].Present) {
    if (!rateInitialized1) {
      lastTemp1 = DT[1].Temperature;
      lastRateCalcMillis1 = nowMillis;
      rateInitialized1 = true;
      ch1_rate = 0.0f;
    } else {
      float dtSec = (nowMillis - lastRateCalcMillis1) / 1000.0f;
      if (dtSec >= 1.0f) {
        float rawRate = ((DT[1].Temperature - lastTemp1) / dtSec) * 60.0f; // °C/min
        ch1_rate = (ch1_rate * 0.70f) + (rawRate * 0.30f);
        lastTemp1 = DT[1].Temperature;
        lastRateCalcMillis1 = nowMillis;
      }
    }
  } else {
    ch1_rate = 0.0f;
    ch1_landing_active = false;
  }

  // Check emergency thermal cutoff
  if ((DT[0].Present && DT[0].Temperature >= MAX_SAFE_TEMP) ||
      (DT[1].Present && DT[1].Temperature >= MAX_SAFE_TEMP)) {
    if (tuningStatus.state == TUNER_SETTLING || tuningStatus.state == TUNER_TESTING) {
      tuningStatus.state = TUNER_ERROR;
      tuningStatus.message = "Emergency Stop: Temperature exceeded safe cutoff limit (31°C)!";
      tuningRequired0 = false;
      tuningRequired1 = false;
      Output0 = 0;
      Output1 = 0;
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      return;
    }
  }

  // --- TUNING ACTIVE ON CH0 ---
  if (tuningRequired0 && tuningStatus.heater == 0) {
    // 1. Force CH1 strictly OFF during CH0 tuning to avoid thermal & electrical interference
    Output1 = 0;
    ledcWrite(1, 0);

    if (DT[0].Present) {
      Input0 = DT[0].Temperature;
      uint8_t status = tuner0.Run();
      tuningStatus.currentTemp = Input0;
      tuningStatus.currentOutput = Output0;

      unsigned long elapsedSec = (millis() - tuningStatus.startMillis) / 1000;
      if (elapsedSec < tuningStatus.settleTime) {
        tuningStatus.state = TUNER_SETTLING;
        tuningStatus.message = "Settling baseline (all heaters OFF)...";
      } else {
        tuningStatus.state = TUNER_TESTING;
        tuningStatus.message = "Applying step output & analyzing response...";
      }

      uint32_t totalSec = tuningStatus.settleTime + tuningStatus.testTime;
      if (status == tuner0.tunings || status == tuner0.runPid || (elapsedSec >= totalSec && elapsedSec > 0)) {
        computeDewParallelTunings(tuner0, tuningStatus.tuningRule, Kp, Ki, Kd);
        tuningStatus.calc_Kp = Kp;
        tuningStatus.calc_Ki = Ki;
        tuningStatus.calc_Kd = Kd;
        tuningStatus.processGain = tuner0.GetProcessGain();
        tuningStatus.deadTime = tuner0.GetDeadTime();
        tuningStatus.tau = tuner0.GetTau();
        tuningStatus.state = TUNER_COMPLETE;
        tuningStatus.message = "Auto-Tuning Complete! Responsive parallel parameters calculated.";
        tuningRequired0 = false;
        // Turn heater output OFF while awaiting user review
        Output0 = 0;
        ledcWrite(0, 0);
        Serial.printf(">>> Tuning Complete CH0! Calculated Responsive: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", Kp, Ki, Kd);
      }
    } else {
      // Sensor disconnected during test
      tuningStatus.state = TUNER_ERROR;
      tuningStatus.message = "Sensor disconnected during tuning!";
      tuningRequired0 = false;
      Output0 = 0;
      ledcWrite(0, 0);
    }
    return;
  }

  // --- TUNING ACTIVE ON CH1 ---
  if (tuningRequired1 && tuningStatus.heater == 1) {
    // 1. Force CH0 strictly OFF during CH1 tuning to avoid thermal & electrical interference
    Output0 = 0;
    ledcWrite(0, 0);

    if (DT[1].Present) {
      Input1 = DT[1].Temperature;
      uint8_t status = tuner1.Run();
      tuningStatus.currentTemp = Input1;
      tuningStatus.currentOutput = Output1;

      unsigned long elapsedSec = (millis() - tuningStatus.startMillis) / 1000;
      if (elapsedSec < tuningStatus.settleTime) {
        tuningStatus.state = TUNER_SETTLING;
        tuningStatus.message = "Settling baseline (all heaters OFF)...";
      } else {
        tuningStatus.state = TUNER_TESTING;
        tuningStatus.message = "Applying step output & analyzing response...";
      }

      uint32_t totalSec = tuningStatus.settleTime + tuningStatus.testTime;
      if (status == tuner1.tunings || status == tuner1.runPid || (elapsedSec >= totalSec && elapsedSec > 0)) {
        computeDewParallelTunings(tuner1, tuningStatus.tuningRule, Kp, Ki, Kd);
        tuningStatus.calc_Kp = Kp;
        tuningStatus.calc_Ki = Ki;
        tuningStatus.calc_Kd = Kd;
        tuningStatus.processGain = tuner1.GetProcessGain();
        tuningStatus.deadTime = tuner1.GetDeadTime();
        tuningStatus.tau = tuner1.GetTau();
        tuningStatus.state = TUNER_COMPLETE;
        tuningStatus.message = "Auto-Tuning Complete! Responsive parallel parameters calculated.";
        tuningRequired1 = false;
        // Turn heater output OFF while awaiting user review
        Output1 = 0;
        ledcWrite(1, 0);
        Serial.printf(">>> Tuning Complete CH1! Calculated Responsive: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", Kp, Ki, Kd);
      }
    } else {
      // Sensor disconnected during test
      tuningStatus.state = TUNER_ERROR;
      tuningStatus.message = "Sensor disconnected during tuning!";
      tuningRequired1 = false;
      Output1 = 0;
      ledcWrite(1, 0);
    }
    return;
  }

  // --- TUNER COMPLETE AWAITING DECISION ---
  if (tuningStatus.state == TUNER_COMPLETE) {
    // Hold outputs safe (0%) until user applies or discards
    Output0 = 0;
    Output1 = 0;
    return;
  }

  // --- NORMAL MAIN OPERATION (Both Channels Closed-Loop PID) ---
  float currentAmbient = 10.0f;
  if (TempC.length() > 0 && TempC.toFloat() > -50.0f && TempC.toFloat() < 60.0f) {
    currentAmbient = TempC.toFloat();
  }

  if (DT[0].Present && heaterEnabled0) {
    Input0 = DT[0].Temperature;
    heater0.Compute();
    Output0 = computeSoftLandingOutput(0, Input0, Setpoint0, H0_Kp, H0_Kd, currentAmbient, Output0);
    updateAdaptiveModel(0, Input0, Setpoint0, ch0_rate, currentAmbient, Output0);
  } else {
    Output0 = 0;
    ch0_landing_active = false;
    engineState0 = LEARNING_STATE_STANDBY;
  }

  if (DT[1].Present && heaterEnabled1) {
    Input1 = DT[1].Temperature;
    heater1.Compute();
    Output1 = computeSoftLandingOutput(1, Input1, Setpoint1, H1_Kp, H1_Kd, currentAmbient, Output1);
    updateAdaptiveModel(1, Input1, Setpoint1, ch1_rate, currentAmbient, Output1);
  } else {
    Output1 = 0;
    ch1_landing_active = false;
    engineState1 = LEARNING_STATE_STANDBY;
  }
}

int returnPWR(int PWMVal)
{
  int myvalue = ((float) PWMVal/255)*100;
  return myvalue;
}
