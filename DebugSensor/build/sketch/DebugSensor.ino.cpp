#include <Arduino.h>
#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
#include "DHT.h"
#include "FS.h"
#include "RTClib.h"
#include "SD.h"
#include "SPI.h"
#include <Adafruit_PCF8574.h>
#include <ModbusMaster.h>
#include <Wire.h>


// --- PINOUT ---
#define dhtSensorPin 14
#define RS485_DE 27
#define RS485_RE 32
#define BATTERY_PIN 33
#define VRM_PIN 13

// --- INSTANCIAS ---
ModbusMaster soilSensor;
Adafruit_PCF8574 mux;
RTC_DS3231 rtc;
DHT dht(dhtSensorPin, DHT21);

// --- VARIABLES ---
float airTemperature, airHumidity, soilTemperature, soilMoisture;
float bateria;

// --- FUNCIONES DE CONTROL DE ENERGÍA ---

// Encender la VRM (5V y 3.3V)
#line 31 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void turnOnVRM();
#line 39 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void turnOffVRM();
#line 46 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void preTransmission();
#line 51 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void postTransmission();
#line 56 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void setup();
#line 109 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void loop();
#line 31 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\DebugSensor\\DebugSensor.ino"
void turnOnVRM() {
  Serial.println("Encender VRM");
  pinMode(VRM_PIN, OUTPUT);
  digitalWrite(VRM_PIN, LOW); // LOW activa el VRM (según io.ino)
  delay(500);
}

// Apagar la VRM (5V y 3.3V)
void turnOffVRM() {
  Serial.println("Apagar VRM");
  pinMode(VRM_PIN, OUTPUT);
  digitalWrite(VRM_PIN, HIGH); // HIGH apaga el VRM
}

// Configuración RS485
void preTransmission() {
  digitalWrite(RS485_DE, HIGH);
  digitalWrite(RS485_RE, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_DE, LOW);
  digitalWrite(RS485_RE, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- DebugSensor Start ---");

  // 1. LEER BATERIA ANTES DE ENCENDER VRM (Bug Hardware)
  // Es necesario leer la batería con todo apagado para evitar errores
  bateria = analogRead(BATTERY_PIN) * 0.004992;
  Serial.print("Battery (Pre-VRM): ");
  Serial.print(bateria);
  Serial.println(" V");

  // 2. Encender sistema
  turnOnVRM();
  Wire.begin();

  // 3. Inicializar RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  } else {
    Serial.println("RTC found");
    // Descomentar la siguiente línea para actualizar la hora con la de
    // compilación rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    DateTime now = rtc.now();
    Serial.print("RTC Time: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
  }

  // 4. Inicializar Sensores
  dht.begin();

  // RS485 / Modbus
  Serial2.begin(4800, SERIAL_8N1, 25, 26); // RO -> IO25, DI -> IO26
  pinMode(RS485_DE, OUTPUT);
  pinMode(RS485_RE, OUTPUT);
  digitalWrite(RS485_DE, LOW);
  digitalWrite(RS485_RE, LOW);

  soilSensor.begin(1, Serial2);
  soilSensor.preTransmission(preTransmission);
  soilSensor.postTransmission(postTransmission);
}

void loop() {
  Serial.println("\n> Reading Sensors...");

  // Lectura DHT
  airTemperature = dht.readTemperature();
  airHumidity = dht.readHumidity();

  // Lectura Soil Sensor (Modbus)
  uint8_t result;
  uint16_t data[3];

  // Limpiar variables de suelo antes de leer
  soilMoisture = -1;
  soilTemperature = -1;

  result = soilSensor.readHoldingRegisters(0x0000, 3);
  if (result == soilSensor.ku8MBSuccess) {
    data[0] = soilSensor.getResponseBuffer(0); // Humedad
    data[1] = soilSensor.getResponseBuffer(1); // Temperatura
    data[2] = soilSensor.getResponseBuffer(2); // Conductividad

    soilMoisture = data[0] * 0.1;
    soilTemperature = (int16_t)data[1] * 0.1;
  } else {
    Serial.print("Modbus Error: ");
    Serial.println(result, HEX);
  }

  // Mostrar Resultados
  Serial.print("Air Temp: ");
  Serial.print(airTemperature);
  Serial.print(" C | ");
  Serial.print("Air Hum: ");
  Serial.print(airHumidity);
  Serial.println(" %");

  Serial.print("Soil Temp: ");
  Serial.print(soilTemperature);
  Serial.print(" C | ");
  Serial.print("Soil Moist: ");
  Serial.print(soilMoisture);
  Serial.println(" %RH");

  Serial.print("Battery (Cached): ");
  Serial.print(bateria);
  Serial.println(" V");

  delay(1000);
}

