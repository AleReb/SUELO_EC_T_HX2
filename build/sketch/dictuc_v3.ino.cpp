#include <Arduino.h>
#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\dictuc_v3.ino"
// ----------------------------------------
/*
sprintf(enlace,
"http://api-sensores.cmasccp.cl/insertarMedicion?times=%s&idsSensores=%d,%d,%d,%d,%d,%d&idsVariables=%d,%d,%d,%d,%d,%d&valores=%s,%s,%s,%s,%s,%d",
        timestamp[i],
        76,76,81,81,86,91,
        3,6,14,3,4,15,
        airTemperatureLog[i],airHumidityLog[i],soilMoistureLog[i],soilTemperatureLog[i],battery[i],signalValue);
*/

#define stationId 9
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

#line 85 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\dictuc_v3.ino"
void setup();
#line 175 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\dictuc_v3.ino"
void loop();
#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void readCache();
#line 8 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void saveDataToSD();
#line 22 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void checkFile();
#line 51 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void writeFile(fs::FS &fs, const char *path, const char *message);
#line 69 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void appendFile(fs::FS &fs, const char *path, const char *message);
#line 85 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
#line 116 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void createDir(fs::FS &fs, const char *path);
#line 125 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void removeDir(fs::FS &fs, const char *path);
#line 134 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void readFile(fs::FS &fs, const char *path);
#line 150 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
int countFileRows(fs::FS &fs, const char *path);
#line 169 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void renameFile(fs::FS &fs, const char *path1, const char *path2);
#line 178 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void deleteFile(fs::FS &fs, const char *path);
#line 187 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void testFileIO(fs::FS &fs, const char *path);
#line 228 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
bool clearFile(fs::FS &fs, const char *path);
#line 238 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void clearCache();
#line 258 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void createFile(fs::FS &fs, const char *path);
#line 274 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void checkSD();
#line 306 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void fastCheckSD();
#line 2 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void checkRTC();
#line 10 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void checkTime();
#line 30 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void blinkGreenLed();
#line 37 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxAllLed();
#line 47 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxRedLed();
#line 57 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxYellowLed();
#line 67 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxGreenLed();
#line 77 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxOffLed();
#line 87 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void muxCycleLeds();
#line 125 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
void scanAddresses();
#line 2 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\io.ino"
void turnOnVRM();
#line 11 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\io.ino"
void turnOffVRM();
#line 18 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\io.ino"
void print_wakeup_reason();
#line 16 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
void sendData();
#line 81 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
int signalQuality();
#line 105 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
void readResponse(char *buffer, int maxLength);
#line 120 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
int extractSignalQuality(char *response);
#line 131 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
bool sendCache();
#line 280 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
bool connectGSM();
#line 290 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
bool connectGPRS();
#line 305 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
bool closeGPRS();
#line 311 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
bool comandoAT(const char *ATcommand, const char *resp_correcta, unsigned int tiempo);
#line 11 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void preTransmission();
#line 16 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void postTransmission();
#line 22 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void beginRS485();
#line 34 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
bool readSoilSensor(ModbusMaster &node, int nodeId, float &outTemp, float &outMoisture, float &outEC);
#line 65 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void printAllSensors();
#line 102 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void checkSensors();
#line 131 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void readSensors();
#line 175 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
void debugMode();
#line 85 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\dictuc_v3.ino"
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

#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\datalogger.ino"
void readCache() {
  Serial.println("> Leer Caché: ");
  int filas = countFileRows(SD, "/cache.csv");
  Serial.println(filas);
  readFile(SD, "/cache.csv");
}

void saveDataToSD() {
  dataMessage = String(now.unixtime()) + "," + String(bateria, 4) + "," +
                String(airTemperature, 1) + "," + String(airHumidity, 1) + "," +
                String(soilEC, 1) + "," + String(soilTemperature, 1) + "," +
                String(soilMoisture, 1) + "," + String(soilEC2, 1) + "," +
                String(soilTemperature2, 1) + "," + String(soilMoisture2, 1) +
                "\r\n";
  Serial.print("> Guardando logs: ");
  Serial.println(dataMessage);

  appendFile(SD, "/data.csv", dataMessage.c_str());
  appendFile(SD, "/cache.csv", dataMessage.c_str());
}

void checkFile() {
  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  Serial.print("  Check data.csv  ");
  File file = SD.open("/data.csv");
  if (!file) {
    Serial.print("File doesn't exist  ");
    Serial.print("Creating file...  ");
    writeFile(SD, "/data.csv", "Epoch Time, Battery\r\n");
  } else {
    Serial.print("File already exists");
  }
  file.close();
  Serial.println();

  Serial.print("  Check cache.csv  ");
  file = SD.open("/cache.csv");
  if (!file) {
    Serial.print("File doesn't exist  ");
    Serial.print("Creating file...  ");
    writeFile(SD, "/cache.csv", "");
  } else {
    Serial.print("File already exists");
  }
  file.close();
  Serial.println();
}

// Write to the SD card
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s  ", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.print("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.print("File written  ");
  } else {
    Serial.print("Write failed  ");
  }
  Serial.println();
  file.close();
}

// Append data to the SD card
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("  Appending to file: %s -> ", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listar directorios: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("No se pudo abrir directorio");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("No es un directorio");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n\r", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

int countFileRows(fs::FS &fs, const char *path) {
  Serial.printf("Counting Rows on file: %s  -> ", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return -1;
  }

  int rowCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    rowCount++;
  }

  return rowCount;
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s  ", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

bool clearFile(fs::FS &fs, const char *path) {
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("No se pudo abrir el archivo");
    return false; // Error al abrir el archivo
  }
  file.close();
  return true;
}

void clearCache() {
  Serial.print("Clear cache -> ");
  File file = SD.open("/cache.csv");
  if (!file) {
    Serial.print("File doesn't exist  ");
    Serial.print("Creating file...  ");
    createFile(SD, "/cache.csv");
  } else {
    deleteFile(SD, "/cache.csv");
    delay(100);
    createFile(SD, "/cache.csv");
    File file = SD.open("/cache.csv");
    if (!file) {
      Serial.println("Error creando archivo Caché");
    }
  }
  file.close();
  Serial.println();
}

void createFile(fs::FS &fs, const char *path) {
  Serial.printf("Creando archivo vacío: %s  ", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.print("> Error creando el archivo");
    muxRedLed();
    delay(1000);
    muxOffLed();
    return;
  } else {
    Serial.print("> Archivo creado exitosamente");
  }
  // Serial.println();
  file.close();
}

void checkSD() {
  if (!SD.begin(4)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No hay SD");
    return;
  }

  Serial.print("Tipo de SD: ");
  if (cardType == CARD_MMC) {
    Serial.print("MMC");
  } else if (cardType == CARD_SD) {
    Serial.print("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.print("SDHC");
  } else {
    Serial.print("UNKNOWN");
  }

  Serial.print(" - ");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(SD, "/", 0);
  Serial.printf("Total space: %lluMB  ", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void fastCheckSD() {
  Serial.print("> Check SD: ");
  if (!SD.begin(4)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No hay SD");
    return;
  }

  Serial.print("Tipo de SD: ");
  if (cardType == CARD_MMC) {
    Serial.print("MMC");
  } else if (cardType == CARD_SD) {
    Serial.print("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.print("SDHC");
  } else {
    Serial.print("UNKNOWN");
  }

  Serial.print(" - ");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB", cardSize);
  Serial.print(" - ");
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\i2c.ino"
//  Verifica la conexión del reloj
void checkRTC(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
}

//  Consulta y muestra la hora del RTC
void checkTime(){
    DateTime now = rtc.now();
    Serial.print("Revisar hora RTC: ");

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print("  ");
    Serial.print(now.hour());
    Serial.print(':');
    Serial.print(now.minute());
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print("  ");
    Serial.print(now.unixtime(), DEC);
    Serial.println();
}

void blinkGreenLed(){
  muxGreenLed();
  delay(200);
  muxOffLed();
  delay(200);
}

void muxAllLed(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, LOW);
  mux.digitalWrite(1, LOW);
  mux.digitalWrite(2, LOW);
}

void muxRedLed(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, LOW);
}

void muxYellowLed(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, LOW);
  mux.digitalWrite(2, HIGH);
}

void muxGreenLed(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, LOW);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
}

void muxOffLed(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
}

void muxCycleLeds(){
  mux.begin(0x20, &Wire);
  mux.pinMode(0, OUTPUT);
  mux.pinMode(1, OUTPUT);
  mux.pinMode(2, OUTPUT);
  mux.digitalWrite(0, LOW);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
  delay(250);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, LOW);
  mux.digitalWrite(2, HIGH);
  delay(250);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, LOW);
  delay(250);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
  delay(250);
  mux.digitalWrite(0, LOW);
  mux.digitalWrite(1, LOW);
  mux.digitalWrite(2, LOW);
  delay(100);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
  delay(200);
  mux.digitalWrite(0, LOW);
  mux.digitalWrite(1, LOW);
  mux.digitalWrite(2, LOW);
  delay(100);
  mux.digitalWrite(0, HIGH);
  mux.digitalWrite(1, HIGH);
  mux.digitalWrite(2, HIGH);
}

void scanAddresses(){
  Wire.begin();

  byte error, address;
  int nDevices;
 
  Serial.print("Direcciones I2C: "); 
  nDevices = 0;
  for(address = 1; address < 127; address++ ){
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0){
      Serial.print("0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.print(" "); 
      nDevices++;
    }
    else if (error==4){
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("N/A");
  Serial.println();
}


#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\io.ino"
//  Encender la VRM (5V y 3.3V)
void turnOnVRM(){
  Serial.println("Encender VRM");
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  delay(500);
}


//  Apagar la VRM (5V y 3.3V)
void turnOffVRM(){
  Serial.println("Apagar VRM");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
}

//  Muestra en serial la causa de booteo
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\modem.ino"
/*
  APIs

  SOIL-01:
  https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=76,76,81,86&idsVariables=3,6,14,14&valores=1,2,3,4
  SOIL-02:
  https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=77,77,82,87&idsVariables=3,6,14,14&valores=1,2,3,4
  SOIL-03:
  https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=78,78,83,88&idsVariables=3,6,14,14&valores=1,2,3,4
  SOIL-04:
  https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=79,79,84,89&idsVariables=3,6,14,14&valores=1,2,3,4
  SOIL-05:
  https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=80,80,85,90&idsVariables=3,6,14,14&valores=1,2,3,4
*/

void sendData() {
  unsigned long modemInstanceStartTime = millis();
  Serial.println("> Send Data");
  turnOnVRM();
  muxYellowLed();
  checkSD();   // Verificamos tarjeta SD
  checkFile(); // Verificamos que existan los CSV

  pinMode(17, INPUT);
  pinMode(16, INPUT);

  myModem.begin(115200, SERIAL_8N1, 17, 16);
  Serial.println("Esperando Modem");
  modemReady = false;
  bool modemConnected = true;

  unsigned long startWaitTime = millis();
  while (!myModem.available()) {
    // Espera que el modem entregue su estado inicial
    if (millis() - startWaitTime > 30000) {
      Serial.println("El modem no contesta");
      modemConnected = false;
      break;
    }
  }

  if (modemConnected) {
    muxGreenLed();
    while (myModem.available()) { // Recibe el estado inicial
      Serial.print((char)myModem.read());
    }
    // comandoAT("AT+COPS=0", "\r\nOK\r\n", 1000);

    for (int i = 0; i < 20; i++) { // Verifica conexión
      if (connectGSM()) {
        modemReady = true;
        break;
      }
      delay(1000);
    }

    signalValue = signalQuality();

    if (modemReady) {
      connectGPRS();
      sendCache();
      closeGPRS();
    } else {
      Serial.println("No hay señal. No borramos Caché.");
      muxRedLed();
      delay(1000);
      muxOffLed();
    }
    modemInstance = false;
  }
  myModem.end();
  digitalWrite(17, LOW);
  digitalWrite(16, LOW);

  for (int i = 0; i < 5; i++)
    blinkGreenLed();
}

//////////////////////////////////////////////////////////////////////////

int signalQuality() {
  myModem.println("AT+CSQ");
  delay(1000);
  if (myModem.available()) {
    char response[100];
    readResponse(response, sizeof(response));
    // Serial.print("Respuesta: ");
    // Serial.println(response);

    Serial.print("Calidad Señal: ");
    // Extraer el valor de la calidad de la señal
    int signalQuality = extractSignalQuality(response);
    if (signalQuality != -1) {
      Serial.println(signalQuality);
    } else {
      Serial.println("Error");
    }
    return signalQuality;
  } else {
    return -1;
  }
}

// Función para leer la respuesta completa del módulo
void readResponse(char *buffer, int maxLength) {
  int index = 0;
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    if (myModem.available() && index < maxLength - 1) {
      char c = myModem.read();
      if (c == '\r' || c == '\n')
        continue; // Ignorar los caracteres de nueva línea
      buffer[index++] = c;
    }
  }
  buffer[index] = '\0'; // Terminar la cadena correctamente
}

// Función para extraer la calidad de la señal de la respuesta
int extractSignalQuality(char *response) {
  char *p = strstr(response, "+CSQ: ");
  if (p != NULL) {
    int rssi;
    if (sscanf(p, "+CSQ: %d,", &rssi) == 1) {
      return rssi;
    }
  }
  return -1; // Indica error
}

bool sendCache() {
  readCache();

  int filas = countFileRows(SD, "/cache.csv");
  Serial.println(filas);

  File file = SD.open("/cache.csv");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  Serial.println("Read from file: ");

  char *timestamp[filas];
  char *battery[filas];
  char *airTemperatureLog[filas];
  char *airHumidityLog[filas];
  char *soilECLog[filas];
  char *soilTemperatureLog[filas];
  char *soilMoistureLog[filas];
  char *soilEC2Log[filas];
  char *soilTemperature2Log[filas];
  char *soilMoisture2Log[filas];

  char temporal[100];
  int i = 0;
  int j = 0;
  int k = 0;
  while (file.available()) {
    char n = file.read();
    if (n == ',' || n == '\r') {
      // Si es el fin de un dato, pasar temporal[] a entry para crear un nuevo
      // pointer (creo)
      temporal[i] = '\0';
      // Serial.println(temporal);
      // Serial.print("> ");
      // Serial.print(j);
      // Serial.print("  ");
      // Serial.println(j%4);

      char *entry = strdup(temporal);

      if (j % 10 == 0)
        timestamp[k] = entry;
      else if (j % 10 == 1)
        battery[k] = entry;
      else if (j % 10 == 2)
        airTemperatureLog[k] = entry;
      else if (j % 10 == 3)
        airHumidityLog[k] = entry;
      else if (j % 10 == 4)
        soilECLog[k] = entry;
      else if (j % 10 == 5)
        soilTemperatureLog[k] = entry;
      else if (j % 10 == 6)
        soilMoistureLog[k] = entry;
      else if (j % 10 == 7)
        soilEC2Log[k] = entry;
      else if (j % 10 == 8)
        soilTemperature2Log[k] = entry;
      else if (j % 10 == 9) {
        soilMoisture2Log[k] = entry;
        k++;
      }
      memset(temporal, '\0', 100);
      i = 0;
      j++;
    } else if (n == '\n') {
      // Ignorar los LF
    } else {
      // Si no es CR, LF ni Coma, agregar a temporal[]
      temporal[i] = n;
      i++;
    }
  }
  file.close();
  Serial.println("ID  Timestamp  Batt  Temp  Hum  EC1  SoilT1  SoilH1  EC2  "
                 "SoilT2  SoilH2");
  for (int i = 0; i < filas; i++) {
    Serial.print("Log ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(timestamp[i]);
    Serial.print(" ");
    Serial.print(battery[i]);
    Serial.print(" ");
    Serial.print(airTemperatureLog[i]);
    Serial.print(" ");
    Serial.print(airHumidityLog[i]);
    Serial.print(" ");
    Serial.print(soilECLog[i]);
    Serial.print(" ");
    Serial.print(soilTemperatureLog[i]);
    Serial.print(" ");
    Serial.print(soilMoistureLog[i]);
    Serial.print(" ");
    Serial.print(soilEC2Log[i]);
    Serial.print(" ");
    Serial.print(soilTemperature2Log[i]);
    Serial.print(" ");
    Serial.print(soilMoisture2Log[i]);
    Serial.println();
  }

  for (int i = 0; i < filas; i++) {
    Serial.print("Enviando log ");
    Serial.print(i);
    Serial.println(":");

    char enlace[600];

    sprintf(enlace, "%s%s,%s,%s,%s,%s,%s,0,0,%d,0,0,%s,%s,%s&times=%s",
            BASE_SERVER_URL, airTemperatureLog[i], airHumidityLog[i],
            soilECLog[i], soilTemperatureLog[i], soilMoistureLog[i], battery[i],
            signalValue, soilEC2Log[i], soilTemperature2Log[i],
            soilMoisture2Log[i], timestamp[i]);

    // https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=76,76,81,86&idsVariables=3,6,14,14&valores=1,2,3,4

    Serial.print("URL: ");
    Serial.println(enlace);

    char atCommand[700];

    sprintf(atCommand, "AT+HTTPPARA=\"URL\",\"%s\"", enlace);
    Serial.println();
    Serial.print(atCommand);
    Serial.println();
    comandoAT(atCommand, "OK", 1000);
    delay(1000);

    if (comandoAT("AT+HTTPACTION=0", "\r\nOK\r\n\r\n+HTTPACTION: 0,201,66\r\n",
                  10000))
      Serial.println("AT+HTTPACTION CHECK");
    else {
      muxRedLed();
      delay(1000);
      muxOffLed();
    }

    delay(1000);
    comandoAT("AT+HTTPREAD=66", "any", 1000);
  }

  clearFile(SD, "/cache.csv");

  return true;
}

bool connectGSM() {
  if (!comandoAT("AT", "\r\nOK\r\n", 1000))
    return false;
  if (!comandoAT("AT+CREG?", "\r\n+CREG: 0,5\r\n\r\nOK\r\n", 1000))
    return false;
  if (!comandoAT("AT+CGREG?", "\r\n+CGREG: 0,5\r\n\r\nOK\r\n", 1000))
    return false;
  return true;
}

bool connectGPRS() {
  char atCommand[255];
  sprintf(atCommand, "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
  if (comandoAT(atCommand, "\r\nOK\r\n", 1000))
    Serial.println("APN CHECK");

  if (comandoAT("AT+NETOPEN", "\r\nOK\r\n\r\n+NETOPEN: 0\r\n", 5000))
    Serial.println("AT+NETOPEN CHECK");

  if (comandoAT("AT+HTTPINIT", "\r\nOK\r\n", 5000))
    Serial.println("AT+HTTPINIT CHECK");

  return true;
}

bool closeGPRS() {
  comandoAT("AT+HTTPTERM", "OK", 1000);
  comandoAT("AT+NETCLOSE", "OK", 1000);
  return true;
}

bool comandoAT(const char *ATcommand, const char *resp_correcta,
               unsigned int tiempo) {
  int x = 0;
  bool correcto = false;
  char respuesta[100];
  unsigned long anterior;

  if (strcmp(resp_correcta, "any") == 0) {
    while (myModem.available() > 0)
      myModem.read();
    myModem.println(ATcommand);
    delay(100);
    while (myModem.available() > 0)
      Serial.print((char)myModem.read());
    correcto = true;
  } else {
    memset(respuesta, '\0', 100);
    delay(100);
    while (myModem.available() > 0)
      myModem.read();
    myModem.println(ATcommand);
    x = 0;
    anterior = millis();
    do {
      if (myModem.available() != 0) {
        char c = myModem.read();
        if (x < sizeof(respuesta) - 1) {
          respuesta[x] = c;
          x++;
        }
        if (strstr(respuesta, resp_correcta) != NULL) {
          correcto = true;
        }
      }
    } while (!correcto && ((millis() - anterior) < tiempo));
  }

  respuesta[x] = '\0'; // Asegurarse de que la cadena esté terminada
  Serial.println(respuesta);
  return correcto;
}
#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\sensors.ino"
// =============================================================
//  sensors.ino  —  Lectura de sensores ambientales y de suelo
//  Sensores:
//    - DHT21 (pin 14)         → Temperatura y Humedad del Aire
//    - Sensor Modbus ID:1     → Humedad, Temperatura y EC del Suelo (Sensor 1)
//    - Sensor Modbus ID:2     → Humedad, Temperatura y EC del Suelo (Sensor 2)
//  Bus: RS485 via Serial2 (RX=IO25, TX=IO26), 4800 baud
// =============================================================

// --- Control de dirección RS485 ---
void preTransmission() {
  digitalWrite(RS485_DE, HIGH);
  digitalWrite(RS485_RE, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_DE, LOW);
  digitalWrite(RS485_RE, LOW);
}

// --- Inicializar bus RS485 / Modbus ---
void beginRS485() {
  Serial2.begin(4800, SERIAL_8N1, 25, 26); // RO -> IO25, DI -> IO26
  pinMode(RS485_DE, OUTPUT);
  pinMode(RS485_RE, OUTPUT);
  digitalWrite(RS485_DE, LOW);
  digitalWrite(RS485_RE, LOW);
}

// --- Función auxiliar: lee un sensor de suelo por Modbus y guarda los valores
// ---
//     Parámetros de salida: temperatura, humedad y EC (pasadas por referencia)
//     Devuelve true si la lectura fue exitosa
bool readSoilSensor(ModbusMaster &node, int nodeId, float &outTemp,
                    float &outMoisture, float &outEC) {
  uint8_t result;
  uint16_t data[3];

  result = node.readHoldingRegisters(0x0000, 3); // Lee Humedad, Temp, EC

  if (result == node.ku8MBSuccess) {
    data[0] = node.getResponseBuffer(0); // Humedad
    data[1] = node.getResponseBuffer(1); // Temperatura
    data[2] = node.getResponseBuffer(2); // Electroconductividad

    outMoisture = data[0] * 0.1;      // %RH
    outTemp = (int16_t)data[1] * 0.1; // °C (con signo)
    outEC = data[2];                  // µS/cm

    return true;
  } else {
    outTemp = -1;
    outMoisture = -1;
    outEC = -1;

    Serial.print("  [Sensor ");
    Serial.print(nodeId);
    Serial.print("] Error Modbus: 0x");
    Serial.println(result, HEX);
    return false;
  }
}

// --- Imprime todos los valores por Serial ---
void printAllSensors() {
  Serial.println("--------------------------------------------------");
  Serial.print("Air Temp:    ");
  Serial.print(airTemperature);
  Serial.println(" C");
  Serial.print("Air Hum:     ");
  Serial.print(airHumidity);
  Serial.println(" %");
  Serial.println("  --- Sensor Suelo 1 (Modbus ID:1) ---");
  Serial.print("  Soil Temp:  ");
  Serial.print(soilTemperature);
  Serial.println(" C");
  Serial.print("  Soil Hum:   ");
  Serial.print(soilMoisture);
  Serial.println(" %RH");
  Serial.print("  Soil EC:    ");
  Serial.print(soilEC);
  Serial.println(" uS/cm");
  Serial.println("  --- Sensor Suelo 2 (Modbus ID:2) ---");
  Serial.print("  Soil Temp:  ");
  Serial.print(soilTemperature2);
  Serial.println(" C");
  Serial.print("  Soil Hum:   ");
  Serial.print(soilMoisture2);
  Serial.println(" %RH");
  Serial.print("  Soil EC:    ");
  Serial.print(soilEC2);
  Serial.println(" uS/cm");
  Serial.print("Battery:     ");
  Serial.print(bateria);
  Serial.println(" V");
  Serial.println("--------------------------------------------------");
}

// =============================================================
//  checkSensors()  —  Verificación rápida al primer boot
// =============================================================
void checkSensors() {
  Serial.println("> Check Sensors:");

  // DHT
  dht.begin();
  airTemperature = dht.readTemperature();
  airHumidity = dht.readHumidity();

  // RS485 / Modbus
  beginRS485();

  soilSensor.begin(1, Serial2);
  soilSensor.preTransmission(preTransmission);
  soilSensor.postTransmission(postTransmission);

  soilSensor2.begin(2, Serial2);
  soilSensor2.preTransmission(preTransmission);
  soilSensor2.postTransmission(postTransmission);

  readSoilSensor(soilSensor, 1, soilTemperature, soilMoisture, soilEC);
  delay(500);
  readSoilSensor(soilSensor2, 2, soilTemperature2, soilMoisture2, soilEC2);

  printAllSensors();
}

// =============================================================
//  readSensors()  —  Lectura principal (cada 30 minutos)
// =============================================================
void readSensors() {
  Serial.println("> Read Sensors");

  // Batería
  bateria = analogRead(33) * 0.004992;

  // Hora RTC
  now = rtc.now();

  // DHT
  dht.begin();
  airTemperature = dht.readTemperature();
  airHumidity = dht.readHumidity();

  if (isnan(airTemperature))
    airTemperature = -1;
  if (isnan(airHumidity))
    airHumidity = -1;

  // RS485 / Modbus — inicializar bus
  beginRS485();

  soilSensor.begin(1, Serial2);
  soilSensor.preTransmission(preTransmission);
  soilSensor.postTransmission(postTransmission);

  soilSensor2.begin(2, Serial2);
  soilSensor2.preTransmission(preTransmission);
  soilSensor2.postTransmission(postTransmission);

  // Leer ambos sensores
  readSoilSensor(soilSensor, 1, soilTemperature, soilMoisture, soilEC);
  delay(500); // Pausa entre nodos en el mismo bus
  readSoilSensor(soilSensor2, 2, soilTemperature2, soilMoisture2, soilEC2);

  // Imprimir todo por Serial
  printAllSensors();
}

// =============================================================
//  debugMode()  —  Modo debug activado por botón GPIO 0
//  Lee y muestra sensores cada 30 segundos durante 5 minutos
//  Guarda cada lectura en la SD
// =============================================================
void debugMode() {
  Serial.println("\n\r==================================================");
  Serial.println("  >>> MODO DEBUG ACTIVADO (Boton GPIO 0) <<<");
  Serial.println("  Leyendo sensores cada 30s durante 5 minutos");
  Serial.println("==================================================\n\r");

  turnOnVRM();
  Wire.begin();
  muxYellowLed(); // Led amarillo = modo debug
  checkRTC();
  fastCheckSD();
  checkFile();

  // Inicializar sensores una sola vez
  dht.begin();
  beginRS485();
  soilSensor.begin(1, Serial2);
  soilSensor.preTransmission(preTransmission);
  soilSensor.postTransmission(postTransmission);
  soilSensor2.begin(2, Serial2);
  soilSensor2.preTransmission(preTransmission);
  soilSensor2.postTransmission(postTransmission);

  // 5 minutos = 300 segundos / 30 segundos = 10 lecturas
  for (int ciclo = 1; ciclo <= 10; ciclo++) {
    Serial.print("\n\r--- Debug Lectura ");
    Serial.print(ciclo);
    Serial.println(" / 10 ---");

    // Batería
    bateria = analogRead(33) * 0.004992;

    // RTC
    now = rtc.now();

    // DHT
    airTemperature = dht.readTemperature();
    airHumidity = dht.readHumidity();
    if (isnan(airTemperature))
      airTemperature = -1;
    if (isnan(airHumidity))
      airHumidity = -1;

    // Suelo sensor 1
    readSoilSensor(soilSensor, 1, soilTemperature, soilMoisture, soilEC);
    delay(500);
    // Suelo sensor 2
    readSoilSensor(soilSensor2, 2, soilTemperature2, soilMoisture2, soilEC2);

    // Mostrar por Serial
    printAllSensors();

    // Guardar en SD
    saveDataToSD();

    // Parpadeo verde = lectura OK
    blinkGreenLed();
    muxYellowLed(); // Volver a amarillo

    // Esperar 30 segundos (excepto en la última lectura)
    if (ciclo < 10) {
      Serial.println("Esperando 30 segundos...");
      delay(30000);
    }
  }

  Serial.println("\n\r==================================================");
  Serial.println("  >>> FIN MODO DEBUG <<<");
  Serial.println("==================================================\n\r");

  for (int i = 0; i < 5; i++)
    blinkGreenLed();
  muxOffLed();
  turnOffVRM();
}

