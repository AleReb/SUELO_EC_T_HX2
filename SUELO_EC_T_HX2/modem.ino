
//  APIs 

bool comandoATContiene(const char *ATcommand, const char *resp1, const char *resp2,
                       unsigned int tiempo) {
  int x = 0;
  char respuesta[160];
  unsigned long anterior;

  memset(respuesta, '\0', sizeof(respuesta));
  delay(100);
  while (myModem.available() > 0)
    myModem.read();

  myModem.println(ATcommand);
  anterior = millis();
  do {
    if (myModem.available() != 0) {
      char c = myModem.read();
      if (x < sizeof(respuesta) - 1) {
        respuesta[x] = c;
        x++;
      }
      if (strstr(respuesta, resp1) != NULL ||
          (resp2 != NULL && strstr(respuesta, resp2) != NULL)) {
        respuesta[x] = '\0';
        Serial.println(respuesta);
        return true;
      }
    }
  } while ((millis() - anterior) < tiempo);

  respuesta[x] = '\0';
  Serial.println(respuesta);
  return false;
}

bool waitForHttpSuccess(unsigned int timeoutMs) {
  return comandoATContiene("AT+HTTPACTION=0", "+HTTPACTION: 0,200,",
                           "+HTTPACTION: 0,201,", timeoutMs);
}

bool isRegisteredOnNetwork() {
  if (!comandoATContiene("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 1000))
    return false;

  return comandoATContiene("AT+CGREG?", "+CGREG: 0,1", "+CGREG: 0,5", 1000);
}

void sendData() {
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
    if (burstRequested) {
      Serial.println(">>> Burst solicitado! Abortando espera modem...");
      modemConnected = false;
      break;
    }
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
      if (burstRequested) {
        Serial.println(">>> Burst solicitado! Abortando conexión GSM...");
        break;
      }
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
  char url[512];
  int urlLen = snprintf(
      url, sizeof(url),
      "%s%.1f,%.1f,%.4f,0,0,%d,0,0,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f&times=%lu",
      BASE_SERVER_URL, airTemperature, airHumidity, bateria, signalValue,
      soilEC, soilTemperature, soilMoisture, soilEC2, soilTemperature2,
      soilMoisture2, (unsigned long)now.unixtime());
  if (urlLen < 0 || urlLen >= (int)sizeof(url)) {
    Serial.println("URL actual demasiado larga o invalida");
    return false;
  }

  Serial.println("\n--- Enviando Dato Actual ---");
  Serial.print("URL: ");
  Serial.println(url);

  char atCommand[640];
  int atCommandLen = snprintf(atCommand, sizeof(atCommand),
                              "AT+HTTPPARA=\"URL\",\"%s\"", url);
  if (atCommandLen < 0 || atCommandLen >= (int)sizeof(atCommand)) {
    Serial.println("Comando AT demasiado largo");
    return false;
  }
  // Evitar sobrecargar Serial con el commando tan largo, solo url
  comandoAT(atCommand, "OK", 1000);
  delay(1000);

  if (waitForHttpSuccess(10000)) {
    Serial.println("Envio Exitoso");
    comandoAT("AT+HTTPREAD", "any", 1000);
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

    char enlace[512];
    int enlaceLen = snprintf(
        enlace, sizeof(enlace), "%s%s,%s,%s,0,0,%d,0,0,%s,%s,%s,%s,%s,%s&times=%s",
        BASE_SERVER_URL, airTemperatureLog[i], airHumidityLog[i], battery[i],
        signalValue, soilECLog[i], soilTemperatureLog[i], soilMoistureLog[i],
        soilEC2Log[i], soilTemperature2Log[i], soilMoisture2Log[i],
        timestamp[i]);
    if (enlaceLen < 0 || enlaceLen >= (int)sizeof(enlace)) {
      Serial.println("URL de cache demasiado larga, se conserva para reintento");
      return false;
    }

    // https://api-sensores.cmasccp.cl/insertarMedicion?idsSensores=76,76,81,86&idsVariables=3,6,14,14&valores=1,2,3,4

    Serial.print("URL: ");
    Serial.println(enlace);

    char atCommand[640];
    int atCommandLen = snprintf(atCommand, sizeof(atCommand),
                                "AT+HTTPPARA=\"URL\",\"%s\"", enlace);
    if (atCommandLen < 0 || atCommandLen >= (int)sizeof(atCommand)) {
      Serial.println("Comando AT de cache demasiado largo");
      return false;
    }
    Serial.println();
    Serial.print(atCommand);
    Serial.println();
    comandoAT(atCommand, "OK", 1000);
    delay(1000);

    if (waitForHttpSuccess(10000))
      Serial.println("AT+HTTPACTION CHECK");
    else {
      muxRedLed();
      delay(1000);
      muxOffLed();
    }

    delay(1000);
    comandoAT("AT+HTTPREAD", "any", 1000);
  }

  clearFile(SD, "/cache.csv");

  return true;
}

bool connectGSM() {
  if (!comandoAT("AT", "\r\nOK\r\n", 1000))
    return false;
  if (!isRegisteredOnNetwork())
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
  char respuesta[512];
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
    memset(respuesta, '\0', sizeof(respuesta));
    delay(100);
    while (myModem.available() > 0)
      myModem.read();
    myModem.println(ATcommand);
    x = 0;
    anterior = millis();
    do {
      if (myModem.available() != 0) {
        char c = myModem.read();
        if (x < (int)sizeof(respuesta) - 1) {
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
