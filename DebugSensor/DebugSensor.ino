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
#define BUTTON_PIN 0

// --- INSTANCIAS ---
ModbusMaster node1;
ModbusMaster node2;
Adafruit_PCF8574 mux;
RTC_DS3231 rtc;
DHT dht(dhtSensorPin, DHT21);

// --- VARIABLES ---
float airTemperature, airHumidity;
float bateria;

// --- FUNCIONES DE CONTROL DE ENERGÍA ---

// Encender la VRM (5V y 3.3V)
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

// Función auxiliar para leer sensor de suelo
void readSoilSensor(ModbusMaster &node, int id) {
  uint8_t result;
  uint16_t data[3]; // Humedad, Temperatura, Conductividad

  Serial.print("--- Reading Soil Sensor ID ");
  Serial.print(id);
  Serial.println(" ---");

  // Leer 3 registros comenzando en 0x0000
  result = node.readHoldingRegisters(0x0000, 3);

  if (result == node.ku8MBSuccess) {
    data[0] = node.getResponseBuffer(0); // Humedad
    data[1] = node.getResponseBuffer(1); // Temperatura
    data[2] = node.getResponseBuffer(2); // Conductividad

    float humidity = data[0] * 0.1;
    float temperature = (int16_t)data[1] * 0.1;
    float conductivity = data[2];

    Serial.print("  Soil Temp: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("  Soil Hum:  ");
    Serial.print(humidity);
    Serial.println(" %RH");

    Serial.print("  Soil EC:   ");
    Serial.print(conductivity);
    Serial.println(" uS/cm");
  } else {
    Serial.print("  Modbus Error: ");
    Serial.println(result, HEX);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
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
   //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

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

  // RS485 / Modbus Initialisation
  Serial2.begin(4800, SERIAL_8N1, 25, 26); // RO -> IO25, DI -> IO26
  pinMode(RS485_DE, OUTPUT);
  pinMode(RS485_RE, OUTPUT);
  digitalWrite(RS485_DE, LOW);
  digitalWrite(RS485_RE, LOW);

  // Configurar nodos Modbus (ID 1 y ID 2)
  node1.begin(1, Serial2);
  node1.preTransmission(preTransmission);
  node1.postTransmission(postTransmission);

  node2.begin(2, Serial2);
  node2.preTransmission(preTransmission);
  node2.postTransmission(postTransmission);
}

void loop() {
  Serial.println("\n> Reading Environment...");

  // Lectura DHT
  airTemperature = dht.readTemperature();
  airHumidity = dht.readHumidity();

  Serial.print("Air Temp: ");
  Serial.print(airTemperature);
  Serial.print(" C | ");
  Serial.print("Air Hum: ");
  Serial.print(airHumidity);
  Serial.println(" %");

  // Lectura Sensores de Suelo
  readSoilSensor(node1, 1);
  delay(500); // Pequeña pausa entre lecturas de bus
  readSoilSensor(node2, 2);

  Serial.print("Battery (Cached): ");
  Serial.print(bateria);
  Serial.println(" V");

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
  // Esperar 5 segundos o hasta botón presionado
  Serial.print("Waiting...");
  for (int i = 0; i < 50; i++) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("\n! Button Pressed !");
      while (digitalRead(BUTTON_PIN) == LOW)
        delay(10); // Wait for release
      delay(100);  // Debounce
      break;
    }
    if (i % 10 == 0)
      Serial.print(".");
    delay(100);
  }
  Serial.println();
}
