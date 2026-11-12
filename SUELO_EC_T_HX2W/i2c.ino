// Consola disponible al encender o reiniciar, antes del ciclo de medicion.
void serialClockConsole() {
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) return;
  turnOnVRM();
  Wire.begin();
  bool available = rtc.begin();
  Serial.println("Consola RTC (15 segundos): HORA, FECHA AAAA-MM-DD HH:MM:SS, SALIR");
  if (!available) Serial.println("ERROR: RTC no disponible");
  char line[48];
  size_t length = 0;
  bool overflow = false;
  unsigned long start = millis();
  while (millis() - start < 15000UL) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\r') continue;
      if (c != '\n') {
        if (length < sizeof(line) - 1 && !overflow) line[length++] = c;
        else overflow = true;
        continue;
      }
      line[length] = '\0';
      if (overflow) {
        Serial.println("ERROR: comando demasiado largo");
      } else if (strcmp(line, "SALIR") == 0) {
        turnOffVRM();
        return;
      } else if (strcmp(line, "HORA") == 0 && available) {
        checkTime();
      } else if (strncmp(line, "FECHA ", 6) == 0 && available) {
        const char *s = line + 6;
        bool valid = strlen(s) == 19;
        for (int i = 0; valid && i < 19; ++i) {
          char expected = i == 4 || i == 7 ? '-' : i == 10 ? ' ' :
                          i == 13 || i == 16 ? ':' : 0;
          valid = expected ? s[i] == expected : s[i] >= '0' && s[i] <= '9';
        }
        if (valid) {
          int year = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + s[3]-'0';
          int month = (s[5]-'0')*10 + s[6]-'0';
          int day = (s[8]-'0')*10 + s[9]-'0';
          int hour = (s[11]-'0')*10 + s[12]-'0';
          int minute = (s[14]-'0')*10 + s[15]-'0';
          int second = (s[17]-'0')*10 + s[18]-'0';
          valid = year >= 2000 && year <= 2099 && month >= 1 && month <= 12 &&
                  day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59;
          if (valid) {
            DateTime value(year, month, day, hour, minute, second);
            valid = value.isValid();
            if (valid) {
              rtc.adjust(value);
              DateTime actual = rtc.now();
              if (actual.unixtime() >= value.unixtime() &&
                  actual.unixtime() - value.unixtime() <= 2) {
                Serial.println("RTC actualizado correctamente");
                checkTime();
              } else Serial.println("ERROR: no se pudo verificar el RTC");
            }
          }
        }
        if (!valid) Serial.println("ERROR: fecha invalida; use FECHA AAAA-MM-DD HH:MM:SS");
      } else {
        Serial.println(available ? "ERROR: comando desconocido" : "ERROR: RTC no disponible");
      }
      length = 0;
      overflow = false;
    }
    delay(10);
  }
  turnOffVRM();
}

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

