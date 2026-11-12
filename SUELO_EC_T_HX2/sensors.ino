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
  Serial.print("Timestamp:   ");
  Serial.println(now.unixtime());
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
//  waitWithYellowBlink()  —  Espera con parpadeo amarillo
//  Parpadea LED amarillo 1 segundo cada 10 segundos durante
//  el tiempo indicado en milisegundos
// =============================================================
void waitWithYellowBlink(unsigned long waitMs) {
  unsigned long start = millis();
  unsigned long blinkInterval = 10000; // 10 segundos entre parpadeos
  unsigned long lastBlink = start;

  muxOffLed(); // Apagar LED al inicio de la espera

  while ((millis() - start) < waitMs) {
    // Cada 10 segundos, parpadear amarillo 1 segundo
    if ((millis() - lastBlink) >= blinkInterval) {
      muxYellowLed();
     Serial.println("prendiendo amarillo... esperando 10 segundos");
      delay(1000); // 1 segundo encendido
      muxOffLed();
      lastBlink = millis();
    }
    delay(100); // Pequeña pausa para no saturar el CPU
  }
}

// =============================================================
//  burstMode()  —  Modo burst activado por botón GPIO 0
//  Lee y guarda sensores cada 5 minutos durante 3 horas
//  Parpadea LED amarillo cada 10s como indicador de modo burst
//  LED verde encendido durante cada guardado
// =============================================================
void burstMode() {
  muxCycleLeds();
  muxCycleLeds();
  const int totalCiclos = 36;                    // 36 lecturas
  const unsigned long intervaloMs = 300000UL;    // 5 minutos = 300000 ms

  Serial.println("\n\r==================================================");
  Serial.println("  >>> MODO BURST ACTIVADO (Boton GPIO 0) <<<");
  Serial.print("  Guardando cada 5 min durante 3 horas (");
  Serial.print(totalCiclos);
  Serial.println(" lecturas)");
  Serial.println("==================================================\n\r");

  turnOnVRM();
  Wire.begin();
  blinkYellowLed(); // Parpadeo amarillo = modo burst activo
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

  for (int ciclo = 1; ciclo <= totalCiclos; ciclo++) {
    Serial.print("\n\r--- Burst Lectura ");
    Serial.print(ciclo);
    Serial.print(" / ");
    Serial.print(totalCiclos);
    Serial.println(" ---");

    // LED verde = leyendo/guardando
    muxGreenLed();

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

    // Parpadeo verde = guardado OK
    muxOffLed();
    delay(200);
    blinkGreenLed();

    // Esperar 5 minutos con parpadeo amarillo (excepto en la última lectura)
    if (ciclo < totalCiclos) {
      Serial.println("Esperando 5 minutos (parpadeo amarillo cada 10s)...");
      waitWithYellowBlink(intervaloMs);
    }
  }

  Serial.println("\n\r==================================================");
  Serial.println("  >>> FIN MODO BURST (3 horas completadas) <<<");
  Serial.println("==================================================\n\r");

  for (int i = 0; i < 5; i++)
    blinkGreenLed();
  muxOffLed();
  turnOffVRM();
}
