// ----------------------------------------
/*
sprintf(enlace,
"http://api-sensores.cmasccp.cl/insertarMedicion?times=%s&idsSensores=%d,%d,%d,%d,%d,%d&idsVariables=%d,%d,%d,%d,%d,%d&valores=%s,%s,%s,%s,%s,%d",
        timestamp[i],
        76,76,81,81,86,91,
        3,6,14,3,4,15,
        airTemperatureLog[i],airHumidityLog[i],soilMoistureLog[i],soilTemperatureLog[i],battery[i],signalValue);
*/

#define stationId 10
#define firmwareVersion 0.3

#if stationId == 9
const char *BASE_SERVER_URL =
    "http://api-sensores.cmasccp.cl/"
    "insertarMedicion?idsSensores=994,994,995,995,995,996,997,997,997,997,997,"
    "998,998,998&idsVariables=3,6,2,3,14,4,11,12,15,45,46,2,3,14&valores=";
#elif stationId == 10
const char *BASE_SERVER_URL =
    "http://api-sensores.cmasccp.cl/"
    "insertarMedicion?idsSensores=999,999,1000,1000,1000,1001,1002,1002,1002,"
    "1002,1002,1003,1003,1003&idsVariables=3,6,2,3,14,4,11,12,15,45,46,2,3,14&"
    "valores=";
#elif stationId == 11
const char *BASE_SERVER_URL =
    "http://api-sensores.cmasccp.cl/"
    "insertarMedicion?idsSensores=1004,1004,1005,1005,1005,1006,1007,1007,1007,"
    "1007,1007,1008,1008,1008&idsVariables=3,6,2,3,14,4,11,12,15,45,46,2,3,14&"
    "valores=";
#else
#error "STATION_ID no esta definido correctamente para las nuevas estaciones."
#endif

// ----------------------------------------

// --- PINOUT ---
#define myModem Serial1
#define dhtSensorPin 14
#define RS485_DE 27
#define RS485_RE 32

// --- LIBRERIAS ---
#include "DHT.h" // Instalar
#include "FS.h"
#include "RTClib.h" // Instalar
#include "SD.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include <Adafruit_PCF8574.h> // Instalar
#include <ModbusMaster.h>     // Instalar
#include <Wire.h>

// Instancias
ModbusMaster soilSensor;  // Sensor 1 (Modbus ID 1)
ModbusMaster soilSensor2; // Sensor 2 (Modbus ID 2)
Adafruit_PCF8574 mux;
RTC_DS3231 rtc;
DHT dht(dhtSensorPin, DHT21);

// Variables globales
DateTime now;
String dataMessage;
float airTemperature, airHumidity;
float soilTemperature, soilMoisture, soilEC;    // Sensor 1
float soilTemperature2, soilMoisture2, soilEC2; // Sensor 2
float bateria;
int signalValue;

// Flags
bool modemReady = false;

// Flag no volátil
RTC_DATA_ATTR bool firstBoot = true;

// APN
char apn[] = "gigsky-02";
// char apn[] = "wap.tmovil.cl";
// char apn[] = "bam.entelpcs.cl";

////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(1024);
  print_wakeup_reason(); // Causa del reinicio

  // --- Modo Debug: si despertó por botón GPIO 0 ---
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    debugMode();
  }

  // --- Primer encendido: verificación completa ---
  if (firstBoot) {
    Serial.print(
        "\n\n --- Estacion de Monitoreo Dictuc 01 (SensorLab-07) --- \n\n");
    Serial.print(" ------------------ id ");
    Serial.print(stationId);
    Serial.print(" | fw ");
    Serial.print(firmwareVersion);
    Serial.print(" -------------------- \n\n");

    Serial.println(
        "\n\r\n\r--------------------------------------------------");
    Serial.println("-> First Boot");
    turnOnVRM();     // Enciende perifericos I2C
    Wire.begin();    // Activa puerto I2C
    scanAddresses(); // Verificamos puertos I2C
    muxCycleLeds();  // Secuencia de Leds de booteo
    checkSD();       // Verificamos tarjeta SD
    checkFile();     // Verificamos que existan los CSV
    clearCache();    // Elimina y reconstruye archivo caché
    checkRTC();      // Verificamos el funcionamiento del RTC
    checkTime();     // Vemos la hora
    checkSensors();  // Verifica lectura sensores
    delay(2000);
    readSensors();  // Leemos sensores
    saveDataToSD(); // Guardamos datos en SD
    sendData();     // Transmitimos datos a servidor
    turnOffVRM();   // Apaga placa hijo
    firstBoot = false;
    Serial.println("\n\r-> End First Boot");
    Serial.println(
        "--------------------------------------------------\n\r\n\r");
  }

  // =============================================================
  //  Ciclo Normal: Lee → Guarda → Envía → Duerme 30 minutos
  // =============================================================
  Serial.println("\n\r--------------------------------------------------");
  Serial.println("-> Ciclo Normal");
  Serial.print("Estacion ");
  Serial.print(stationId);
  Serial.print(" | Firmware ");
  Serial.println(firmwareVersion);

  // Encender perifericos (VRM enciende todos los sensores)
  turnOnVRM();
  Wire.begin();
  muxAllLed();
  delay(250);
  muxOffLed();

  // Ver la hora
  checkRTC();
  checkTime();
  now = rtc.now();

  // Leer y mostrar TODOS los sensores
  readSensors(); // DHT + Suelo1 + Suelo2 + Batería

  // Guardar en SD
  muxGreenLed();
  fastCheckSD();
  checkFile();
  saveDataToSD();

  // Enviar al servidor
  sendData();

  for (int i = 0; i < 3; i++)
    blinkGreenLed();

  // Apagar y dormir 30 minutos
  turnOffVRM();
  Serial.println("-> Deep Sleep 30 minutos");
  Serial.println("--------------------------------------------------\n\r");
  esp_sleep_enable_timer_wakeup(1800000000ULL); // 30 min = 1800 segundos
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Despertar con botón BOOT
  esp_deep_sleep_start();
}

void loop() {
  // No se usa - el ESP32 siempre reinicia desde setup()
}
