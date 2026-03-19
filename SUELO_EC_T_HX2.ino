// ----------------------------------------
/*
NEW LINK STANDARD (Air, Batt, GSM, Soil1, Soil2)

Station 9:
"http://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=994,994,996,997,997,997,997,997,998,998,998,1018,1018,1018&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&valores="

Station 10:
"http://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=999,999,1001,1002,1002,1002,1002,1002,1019,1019,1019,1020,1020,1020&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&valores="

Station 11:
"http://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=1004,1004,1006,1007,1007,1007,1007,1007,1016,1016,1016,1017,1017,1017&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&valores="

Variables:
3,6: Air (T,H)
4: Battery (V)
11,12: GPS (Lat,Lon)
15: Signal (CSQ)
45,46: GPS (Speed,Sats)
2,3,14: Soil (EC,T,H)
*/
// ----------------------------------------

#define stationId 11
#define firmwareVersion 0.40

#if stationId == 9
const char *BASE_SERVER_URL =
  "http://api-sensores.cmasccp.cl/"
  "insertarMedicion?idsSensores=994,994,996,997,997,997,997,997,998,998,998,"
  "1018,1018,1018&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&valores=";
#elif stationId == 10
const char *BASE_SERVER_URL =
  "http://api-sensores.cmasccp.cl/"
  "insertarMedicion?idsSensores=999,999,1001,1002,1002,1002,1002,1002,1019,"
  "1019,1019,1020,1020,1020&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&"
  "valores=";
#elif stationId == 11
const char *BASE_SERVER_URL =
  "http://api-sensores.cmasccp.cl/"
  "insertarMedicion?idsSensores=1004,1004,1006,1007,1007,1007,1007,1007,1016,"
  "1016,1016,1017,1017,1017&idsVariables=3,6,4,11,12,15,45,46,2,3,14,2,3,14&"
  "valores=";
#else
#error "stationId is not defined correctly."
#endif

// ----------------------------------------
// PINOUT
// ----------------------------------------
#define myModem Serial1
#define dhtSensorPin 14
#define RS485_DE 27
#define RS485_RE 32
#define BURST_BUTTON_PIN 0

// ----------------------------------------
// LIBRARIES
// ----------------------------------------
#include "DHT.h"
#include "FS.h"
#include "RTClib.h"
#include "SD.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include <Adafruit_PCF8574.h>
#include <ModbusMaster.h>
#include <Wire.h>

// ----------------------------------------
// INSTANCES
// ----------------------------------------
ModbusMaster soilSensor;
ModbusMaster soilSensor2;
Adafruit_PCF8574 mux;
RTC_DS3231 rtc;
DHT dht(dhtSensorPin, DHT21);

// ----------------------------------------
// GLOBAL DATA
// ----------------------------------------
DateTime now;
String dataMessage;

float airTemperature = -1.0f;
float airHumidity = -1.0f;

float soilTemperature = -1.0f;
float soilMoisture = -1.0f;
float soilEC = -1.0f;

float soilTemperature2 = -1.0f;
float soilMoisture2 = -1.0f;
float soilEC2 = -1.0f;

float bateria = -1.0f;
int signalValue = -1;

// ----------------------------------------
// FLAGS
// ----------------------------------------
bool modemReady = false;
bool burstModeRunning = false;
bool peripheralsOn = false;
bool i2cReady = false;
bool rtcReady = false;
bool sdReady = false;
bool sensorsReady = false;

volatile bool burstRequested = false;
RTC_DATA_ATTR bool firstBoot = true;

// ----------------------------------------
// APN
// ----------------------------------------
char apn[] = "gigsky-02";
// char apn[] = "wap.tmovil.cl";
// char apn[] = "bam.entelpcs.cl";

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------
void printSystemStatus();
void setupBurstButton();
bool isBurstButtonPressed();
bool shouldEnterBurstMode();
void clearBurstRequest();
void IRAM_ATTR onBurstButton();
bool handleBurstRequest(const char *context);
void runBurstMode();
void prepareDeepSleep30Min();
void safeTurnOnPeripherals();
void safeInitI2C();
void safeCheckRTC();
void safeCheckSD();
void safeCheckFiles();
void safeCheckTime();
void safeCheckSensors();
void safeReadSensors();
void safeSaveDataToSD();
void showBootBanner();
void showCycleBanner();
void detachBurstInterruptSafe();

// -----------------------------------------------------------------------------
// ISR
// -----------------------------------------------------------------------------
void IRAM_ATTR onBurstButton() {
  burstRequested = true;
}

// -----------------------------------------------------------------------------
// BURST BUTTON / REQUEST MANAGEMENT
// -----------------------------------------------------------------------------
void setupBurstButton() {
  pinMode(BURST_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BURST_BUTTON_PIN), onBurstButton, FALLING);
}

bool isBurstButtonPressed() {
  return digitalRead(BURST_BUTTON_PIN) == LOW;
}

bool shouldEnterBurstMode() {
  return burstRequested || isBurstButtonPressed();
}

void clearBurstRequest() {
  noInterrupts();
  burstRequested = false;
  interrupts();
}

void detachBurstInterruptSafe() {
  detachInterrupt(digitalPinToInterrupt(BURST_BUTTON_PIN));
}

bool handleBurstRequest(const char *context) {
  if (!shouldEnterBurstMode()) {
    return false;
  }

  Serial.println();
  Serial.println("==================================================");
  Serial.println("BURST REQUEST DETECTED");
  Serial.print("Context: ");
  Serial.println(context);
  Serial.println("Normal transmission will be skipped.");
  Serial.println("==================================================");

  clearBurstRequest();
  burstModeRunning = true;

  safeTurnOnPeripherals();
  safeInitI2C();

  muxCycleLeds();
  muxCycleLeds();
  muxCycleLeds();

  runBurstMode();

  return true;
}

// -----------------------------------------------------------------------------
// SAFE WRAPPERS / STATUS
// -----------------------------------------------------------------------------
void safeTurnOnPeripherals() {
  turnOnVRM();
  peripheralsOn = true;
}

void safeInitI2C() {
  Wire.begin();
  i2cReady = true;
}

void safeCheckRTC() {
  checkRTC();
  rtcReady = true;
}

void safeCheckSD() {
  checkSD();
  sdReady = true;
}

void safeCheckFiles() {
  checkFile();
}

void safeCheckTime() {
  checkTime();
}

void safeCheckSensors() {
  checkSensors();
  sensorsReady = true;
}

void safeReadSensors() {
  readSensors();
  sensorsReady = true;
}

void safeSaveDataToSD() {
  saveDataToSD();
}

void printSystemStatus() {
  Serial.println("--------------- SYSTEM STATUS ----------------");
  Serial.print("Peripherals ON: ");
  Serial.println(peripheralsOn ? "YES" : "NO");
  Serial.print("I2C ready:      ");
  Serial.println(i2cReady ? "YES" : "NO");
  Serial.print("RTC ready:      ");
  Serial.println(rtcReady ? "YES" : "NO");
  Serial.print("SD ready:       ");
  Serial.println(sdReady ? "YES" : "NO");
  Serial.print("Sensors ready:  ");
  Serial.println(sensorsReady ? "YES" : "NO");
  Serial.print("First boot:     ");
  Serial.println(firstBoot ? "YES" : "NO");
  Serial.print("Burst pending:  ");
  Serial.println(shouldEnterBurstMode() ? "YES" : "NO");
  Serial.println("---------------------------------------------");
}

// -----------------------------------------------------------------------------
// UI / BANNERS
// -----------------------------------------------------------------------------
void showBootBanner() {
  Serial.print("\n\n --- Soil Monitoring Station --- \n\n");
  Serial.print(" ------------------ id ");
  Serial.print(stationId);
  Serial.print(" | fw ");
  Serial.print(firmwareVersion);
  Serial.print(" -------------------- \n\n");
}

void showCycleBanner() {
  Serial.println("\n\r--------------------------------------------------");
  Serial.println("-> Normal Cycle");
  Serial.print("Station ");
  Serial.print(stationId);
  Serial.print(" | Firmware ");
  Serial.println(firmwareVersion);
}

// -----------------------------------------------------------------------------
// BURST EXECUTION
// -----------------------------------------------------------------------------
void runBurstMode() {
  Serial.println("Starting burst mode now...");
  burstMode();

  Serial.println("Burst mode finished.");
  printSystemStatus();
}

// -----------------------------------------------------------------------------
// SLEEP
// -----------------------------------------------------------------------------
void prepareDeepSleep30Min() {
  Serial.println("-> Deep Sleep 30 minutes");
  Serial.println("--------------------------------------------------\n\r");

  detachBurstInterruptSafe();

  esp_sleep_enable_timer_wakeup(1800000000ULL);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  esp_deep_sleep_start();
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(1024);

  // Arm burst detection immediately.
  setupBurstButton();

  // Small state reset for current boot.
  burstModeRunning = false;
  peripheralsOn = false;
  i2cReady = false;
  rtcReady = false;
  sdReady = false;
  sensorsReady = false;
  modemReady = false;

  print_wakeup_reason();

  // Detect burst at startup, even if the button is only tapped briefly.
  bool burstHandled = false;
  burstHandled = handleBurstRequest("startup");

  // If it woke from EXT0 and burst has not yet been handled, force burst mode.
  if (!burstHandled && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wakeup source is EXT0, forcing burst mode.");
    burstHandled = handleBurstRequest("wakeup EXT0");
  }

  // ---------------- First Boot ----------------
  if (!burstHandled && firstBoot) {
    showBootBanner();

    Serial.println("\n\r\n\r--------------------------------------------------");
    Serial.println("-> First Boot");

    safeTurnOnPeripherals();
    if (!handleBurstRequest("first boot - after turnOnVRM")) {
      safeInitI2C();
    } else {
      burstHandled = true;
    }

    if (!burstHandled) {
      scanAddresses();
      burstHandled = handleBurstRequest("first boot - after scanAddresses");
    }

    if (!burstHandled) {
      muxCycleLeds();
      burstHandled = handleBurstRequest("first boot - after muxCycleLeds");
    }

    if (!burstHandled) {
      safeCheckSD();
      burstHandled = handleBurstRequest("first boot - after checkSD");
    }

    if (!burstHandled) {
      safeCheckFiles();
      burstHandled = handleBurstRequest("first boot - after checkFile");
    }

    if (!burstHandled) {
      safeCheckRTC();
      burstHandled = handleBurstRequest("first boot - after checkRTC");
    }

    if (!burstHandled) {
      safeCheckTime();
      burstHandled = handleBurstRequest("first boot - after checkTime");
    }

    if (!burstHandled) {
      safeCheckSensors();
      burstHandled = handleBurstRequest("first boot - after checkSensors");
    }

    if (!burstHandled) {
      safeReadSensors();
      burstHandled = handleBurstRequest("first boot - after readSensors");
    }

    if (!burstHandled) {
      safeSaveDataToSD();
      burstHandled = handleBurstRequest("first boot - after saveDataToSD");
    }

    if (!burstHandled) {
      Serial.println("No burst pending. Proceeding to sendData() in first boot.");
      sendData();
      burstHandled = handleBurstRequest("first boot - after sendData");
    } else {
      Serial.println("Burst took priority over first boot transmission.");
    }

    turnOffVRM();
    peripheralsOn = false;
    firstBoot = false;

    Serial.println("\n\r-> End First Boot");
    Serial.println("--------------------------------------------------\n\r\n\r");
  }

  // ---------------- Normal Cycle ----------------
  if (!burstHandled) {
    showCycleBanner();

    safeTurnOnPeripherals();
    if (!handleBurstRequest("normal cycle - after turnOnVRM")) {
      safeInitI2C();
    } else {
      burstHandled = true;
    }

    if (!burstHandled) {
      muxAllLed();
      delay(250);
      muxOffLed();
      burstHandled = handleBurstRequest("normal cycle - after LED init");
    }

    if (!burstHandled) {
      safeCheckRTC();
      burstHandled = handleBurstRequest("normal cycle - after checkRTC");
    }

    if (!burstHandled) {
      safeCheckTime();
      burstHandled = handleBurstRequest("normal cycle - after checkTime");
    }

    if (!burstHandled) {
      safeReadSensors();
      burstHandled = handleBurstRequest("normal cycle - after readSensors");
    }

    if (!burstHandled) {
      muxGreenLed();
      safeCheckSD();
      safeCheckFiles();
      safeSaveDataToSD();
      burstHandled = handleBurstRequest("normal cycle - after saveDataToSD");
    }

    if (!burstHandled) {
      Serial.println("No burst pending. Proceeding to sendData().");
      sendData();
      burstHandled = handleBurstRequest("normal cycle - after sendData");
    } else {
      Serial.println("Burst took priority over normal transmission.");
    }

    for (int i = 0; i < 3; i++) {
      blinkGreenLed();
    }

    turnOffVRM();
    peripheralsOn = false;
  }

  // Final state report before sleep.
  printSystemStatus();

  prepareDeepSleep30Min();
}

void loop() {
  // Not used.
  // The ESP32 restarts from setup() after each wake-up.
}