# — Estación de Monitoreo de Suelo TEMPERATURA, HUMEDAD AIRE Y SUELO y ELECTROCONDUCTIVIDAD SUELO

**Autor de codigo base:** Sebastian Adonay
**Autor de modificaciones:** Alejandro Rebolledo  
**Contacto:** arebolledo@udd.cl 
**Versión Firmware:** 0.3  
**Plataforma:** ESP32 (Arduino Framework)  
**Licencia:** [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/)

---

## Descripción General

Firmware para una red de estaciones de monitoreo ambiental y de suelo, desarrollado para el proyecto DICTUC. Cada estación mide parámetros de temperatura del aire, humedad del aire, y cuenta con soporte para **dos sensores de suelo** simultáneos que miden temperatura, humedad y **electroconductividad (EC)** del suelo. Los datos se guardan en una tarjeta SD y se transmiten a un servidor vía GPRS/GSM cada 30 minutos.

El sistema usa **deep sleep** de forma agresiva para maximizar la vida de la batería: el ESP32 duerme 10 segundos entre ciclos y solo trabaja activamente (mide y guarda) cada **30 minutos** (180 ciclos de 10 s).

Además, se puede usar el pulsador boot (botón 0) para activar un **modo debug** (ráfaga de muestreo) de 5 minutos, guardando los datos con mediciones cada 30 segundos.

---

## Arquitectura del Código

El proyecto está dividido en múltiples archivos `.ino` (compilados juntos por Arduino IDE):

```text
SUELO_EC_T_HX2/
├── SUELO_EC_T_HX2.ino  → Núcleo: Configuración de estación (URL), setup(), bucle de deep sleep
├── sensors.ino         → Lectura DHT21 (aire) + 2 sensores Modbus RS485 (suelo)
├── datalogger.ino      → Gestión SD card: data.csv y cache.csv
├── modem.ino           → Conexión GSM/GPRS y envío HTTP vía comandos AT
├── i2c.ino             → RTC DS3231 y LEDs de estado vía PCF8574 (I2C)
├── io.ino              → Control de energía (VRM) y causa de wakeup
└── DebugSensor/        → Sketch independiente de diagnóstico y desarrollo
    └── DebugSensor.ino
```

---

## Flujo de Ejecución

```text
Wakeup (cada 10s)
    │
    ├─ [Si wakeup por Botón 0] → Ejecuta modo Debug (lee cada 30s por 5 min)
    │
    ├─ [Si firstBoot=true]  Primera vez que arranca
    │       ├── Escanear I2C
    │       ├── Verificar SD / RTC / CSVs
    │       ├── Leer sensores → Guardar SD → Transmitir GPRS
    │       └── firstBoot = false
    │
    ├─ [Cada ciclo]  Parpadear LEDs, verificar hora RTC
    │
    ├─ [Si hora == 00:00 ó 12:00]  Transmitir caché acumulado
    │
    ├─ [Si bootCount == 180]  → Cada 30 minutos
    │       ├── Verificar SD / RTC
    │       ├── Leer sensores (DHT + 2x Suelo Modbus)
    │       ├── Guardar en data.csv y cache.csv
    │       └── Transmitir GPRS (vaciar cache)
    │
    └── Deep sleep 10 segundos → repite
```

---

## Descripción de Módulos

### `SUELO_EC_T_HX2.ino` — Principal
- Define `stationId` (ej. 9, 10, 11) y `firmwareVersion` (0.3).
- Configura la URL de transmisión `BASE_SERVER_URL` específica para cada estación (define los IDs de sensores y variables directamente en la URL).
- Maneja el ciclo de boot con variables en **RTC_DATA_ATTR** (sobreviven deep sleep).
- La lógica principal de inicialización y el ciclo normal de 30 minutos reside en `setup()`.

### `sensors.ino` — Sensores
- **DHT21** (pin 14): temperatura y humedad del aire.
- **2 Sensores de suelo RS485/Modbus** (Serial2, pines 25/26, 4800 baud):
  - Sensor 1 (Modbus ID: 1) y Sensor 2 (Modbus ID: 2).
  - Lee 3 registros desde `0x0000` para cada sensor:
    - `Reg[0]` → Humedad suelo (×0.1 = %RH)
    - `Reg[1]` → Temperatura suelo (int16, ×0.1 = °C)
    - `Reg[2]` → Electroconductividad (µS/cm)
- `RS485_DE` (pin 27) y `RS485_RE` (pin 32) controlan la dirección de comunicación RS485.
- Incluye la función `debugMode()` que lee ambos sensores de forma acelerada por 5 minutos.

### `datalogger.ino` — SD Card
- SD conectada con CS en pin 4.
- **`/data.csv`**: log histórico completo (append, nunca se borra).
- **`/cache.csv`**: datos pendientes de envío (se borra después de transmitir exitosamente).
- Formato CSV actualizado: incluye los 3 parámetros de ambos sensores Modbus (`Epoch Time, Battery, TempAire, HumAire, EC_1, TempSuelo_1, HumSuelo_1, EC_2, TempSuelo_2, HumSuelo_2`).

### `modem.ino` — Comunicación GPRS
- Modem en Serial1 (pines 17/16), 115200 baud.
- APN configurado: `gigsky-02`.
- Transmite los datos mediante peticiones HTTP GET usando comandos AT basándose en la `BASE_SERVER_URL` de la estación configurada.

### `i2c.ino` — RTC y LEDs
- **RTC DS3231**: provee timestamp (Epoch Unix).
- **PCF8574** (dirección I2C `0x20`): expander I2C que controla 3 LEDs RGB.
  - Verde: operación normal / datos enviados exitosamente.
  - Rojo: error.
  - Amarillo: transmitiendo / modo debug activo.

### `io.ino` — Control de Energía
- **VRM (pin 13)**: regulador de voltaje que alimenta los periféricos (I2C, RS485, Sensores).
  - `LOW` = encendido, `HIGH` = apagado (lógica invertida).
- Todos los periféricos se apagan controladamente antes del deep sleep para ahorrar energía.

---

## Hardware (Pinout)

| Pin ESP32 | Función                  |
|-----------|--------------------------|
| 13        | VRM enable (activo LOW)  |
| 14        | DHT21 data               |
| 16        | Modem TX (Serial1)       |
| 17        | Modem RX (Serial1)       |
| 25        | RS485 RO (Serial2 RX)    |
| 26        | RS485 DI (Serial2 TX)    |
| 27        | RS485 DE                 |
| 32        | RS485 RE                 |
| 33        | Battery ADC              |
| 4         | SD Card CS (SPI)         |
| I2C SDA/SCL | RTC DS3231 + PCF8574   |

---

## Librerías Requeridas

| Librería             | Función                     |
|----------------------|-----------------------------|
| `RTClib`             | RTC DS3231                  |
| `Adafruit_PCF8574`   | Expansor I2C (LEDs)         |
| `DHT sensor library` | Sensor DHT21                |
| `ModbusMaster`       | Comunicación RS485/Modbus   |
| `SD`, `SPI`, `FS`    | Tarjeta SD (integradas ESP32) |

---

## Descargo de Responsabilidad

Este código se ofrece **"tal cual"**, sin garantías de ningún tipo, expresas o implícitas. El autor no se hace responsable de daños directos, indirectos, incidentales o de cualquier otro tipo derivados del uso de este software. **El usuario lo utiliza bajo su propio riesgo.**
