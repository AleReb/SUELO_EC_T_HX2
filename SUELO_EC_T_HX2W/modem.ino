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

      bool currentSuccess = sendCurrentData();

      if (!currentSuccess) {
        Serial.println(
            "Fallo al enviar dato actual por GPRS/HTTP. Guardando en CACHE.");
        saveToCache(dataMessage);
      } else {
        Serial.println(
            "Dato actual enviado OK. Revisando si hay cache pendiente...");
        int cacheFilas = countFileRows(SD, "/cache.csv");
        if (cacheFilas > 0) {
          Serial.print("Hay ");
          Serial.print(cacheFilas);
          Serial.println(" datos en cache. Intentando enviar...");
          sendCache(); // Envia y borra archivo cache
        }
      }

      closeGPRS();
    } else {
      Serial.println("No hay red GSM. Guardando en CACHE de respaldo.");
      saveToCache(dataMessage);
      muxRedLed();
      delay(1000);
      muxOffLed();
    }
  } else {
    Serial.println("Falla encendido Modem. Guardando en CACHE de respaldo.");
    saveToCache(dataMessage);
  }
  myModem.end();
  digitalWrite(17, LOW);
  digitalWrite(16, LOW);

  for (int i = 0; i < 5; i++)
    blinkGreenLed();
}

bool sendCurrentData() {
  String url = String(BASE_SERVER_URL) + String(airTemperature, 1) + "," +
               String(airHumidity, 1) + "," + String(soilEC, 1) + "," +
               String(soilTemperature, 1) + "," + String(soilMoisture, 1) +
               "," + String(bateria, 4) + ",0,0," + String(signalValue) +
               ",0,0," + String(soilEC2, 1) + "," +
               String(soilTemperature2, 1) + "," + String(soilMoisture2, 1) +
               "&times=" + String(now.unixtime());

  Serial.println("\n--- Enviando Dato Actual ---");
  Serial.print("URL: ");
  Serial.println(url);

  String atCommand = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
  // Evitar sobrecargar Serial con el commando tan largo, solo url
  comandoAT(atCommand.c_str(), "OK", 1000);
  delay(1000);

  if (comandoAT("AT+HTTPACTION=0", "\r\nOK\r\n\r\n+HTTPACTION: 0,201,66\r\n",
                10000)) {
    Serial.println("Envio Exitoso (201)");
    comandoAT("AT+HTTPREAD=66", "any", 1000);
    return true;
  } else {
    Serial.println("Falla en envio (HTTPACTION)");
    return false;
  }
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