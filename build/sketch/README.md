#line 1 "C:\\Users\\Ale\\Downloads\\dictuc_v3\\README.md"
# — Estación de Monitoreo de Suelo TEMPERATRA HUMEDAD AIRE Y SUELO y ELECTRCONDUCTIVIDAD SUELO

**Autor de codigo base:** Sebastian Adonay
**Autor de modificaciones:** Alejandro Rebolledo  
**Contacto:** [EMAIL_ADDRESS] arebolledo@udd.cl 
**Versión Firmware:** 0.1  
**Plataforma:** ESP32 (Arduino Framework)  
**Licencia:** [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/)

---

## Descripción General

Firmware para una red de estaciones de monitoreo ambiental y de suelo, desarrollado para el proyecto DICTUC. Cada estación mide parámetros de temperatura del aire, humedad del aire, temperatura del suelo y humedad del suelo (con electroconductividad disponible en el sensor), los guarda en una tarjeta SD y los transmite a un servidor vía GPRS/GSM cada 30 minutos.

El sistema usa **deep sleep** de forma agresiva para maximizar la vida de la batería: el ESP32 duerme 10 segundos entre ciclos y solo trabaja activamente (mide y guarda) cada **30 minutos** (180 ciclos de 10 s).

a demas se podra usar el pulsador boot boton 0 para activar una rafaga de muestreo de 5 minutos guardando los datos con mediciones cada 30 segundos.
---

## Arquitectura del Código

El proyecto está dividido en múltiples archivos `.ino` (compilados juntos por Arduino IDE):

```
dictuc_v3/
├── dictuc_v3.ino     → Núcleo: IDs de estación, setup(), bucle de deep sleep
├── sensors.ino       → Lectura DHT21 (aire) + sensor Modbus RS485 (suelo)
├── datalogger.ino    → Gestión SD card: data.csv y cache.csv
├── modem.ino         → Conexión GSM/GPRS y envío HTTP vía comandos AT
├── i2c.ino           → RTC DS3231 y LEDs de estado vía PCF8574 (I2C)
├── io.ino            → Control de energía (VRM) y causa de wakeup
└── DebugSensor/      → Sketch independiente de diagnóstico y desarrollo
    └── DebugSensor.ino
```

---

## Flujo de Ejecución

```
Wakeup (cada 10s)
    │
    ├─ [Si firstBoot=true]  Primera vez que arranca
    │       ├── Escanear I2C
    │       ├── Verificar SD / RTC
    │       ├── Leer sensores → Guardar SD → Transmitir GPRS
    │       └── firstBoot = false
    │
    ├─ [Cada ciclo]  Parpadear LEDs, verificar hora RTC
    │
    ├─ [Si hora == 00:00 ó 12:00]  Transmitir caché acumulado
    │
    ├─ [Si bootCount == 180]  → Cada 30 minutos
    │       ├── Verificar SD / RTC
    │       ├── Leer sensores
    │       └── Guardar en data.csv y cache.csv
    │
    └── Deep sleep 10 segundos → repite
```

---

## Descripción de Módulos

### `dictuc_v3.ino` — Principal
- Define `stationId` y `firmwareVersion` con `#define`
- Usa `#if / #elif` para asignar el array `idsSensores[]` según la estación
- Maneja el ciclo de boot con variables en **RTC_DATA_ATTR** (sobreviven deep sleep)
- `setup()` contiene toda la lógica; `loop()` está vacío

### `sensors.ino` — Sensores
- **DHT21** (pin 14): temperatura y humedad del aire
- **Sensor de suelo RS485/Modbus** (Serial2, pines 25/26, 4800 baud):
  - Dirección Modbus: `1`
  - Lee 3 registros desde `0x0000`:
    - `Reg[0]` → Humedad suelo (×0.1 = %RH)
    - `Reg[1]` → Temperatura suelo (int16, ×0.1 = °C)
    - `Reg[2]` → Electroconductividad (µS/cm) — **leída pero NO guardada actualmente**
- `RS485_DE` (pin 27) y `RS485_RE` (pin 32) controlan dirección RS485

### `datalogger.ino` — SD Card
- SD conectada con CS en pin 4
- **`/data.csv`**: log histórico completo (append, nunca se borra)
- **`/cache.csv`**: datos pendientes de envío (se borra después de transmitir)
- Formato CSV: `epoch_time, battery, airTemp, airHum, soilTemp, soilMoisture`

### `modem.ino` — Comunicación GPRS
- Modem en Serial1 (pines 17/16), 115200 baud
- APN configurado: `gigsky-02`
- Lee el `cache.csv` y envía cada fila a la API via HTTP GET:
  ```
  http://api-sensores.cmasccp.cl/insertarMedicion?
    times=<epoch>
    &idsSensores=<6 IDs>
    &idsVariables=3,6,14,3,4,15
    &valores=airTemp,airHum,soilMoisture,soilTemp,battery,signal
  ```

### `i2c.ino` — RTC y LEDs
- **RTC DS3231**: provee timestamp (Epoch Unix)
- **PCF8574** (dirección I2C `0x20`): expander I2C que controla 3 LEDs RGB
  - Verde: operación normal / datos enviados
  - Rojo: error
  - Amarillo: transmitiendo

### `io.ino` — Control de Energía
- **VRM (pin 13)**: regulador de voltaje que alimenta los periféricos (I2C, RS485)
  - `LOW` = encendido, `HIGH` = apagado (lógica invertida)
- Periféricos se apagan antes del deep sleep para ahorrar energía

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

| Librería           | Función                     |
|--------------------|-----------------------------|
| `RTClib`           | RTC DS3231                  |
| `Adafruit_PCF8574` | Expansor I2C (LEDs)         |
| `DHT sensor library` | Sensor DHT21              |
| `ModbusMaster`     | Comunicación RS485/Modbus   |
| `SD`, `SPI`, `FS`  | Tarjeta SD (integradas ESP32) |

---

## IDs de Sensores por Estación

Los IDs son asignados automáticamente según `stationId`:

| Estación | idsSensores                |
|----------|---------------------------|
| 7        | 76, 76, 81, 81, 86, 91    |
| 8        | 77, 77, 82, 82, 87, 92    |
| 9        | 78, 78, 83, 83, 88, 93    |
| 10       | 79, 79, 84, 84, 89, 94    |
| 11       | 80, 80, 85, 85, 90, 95    |

Las variables transmitidas usan los IDs de variables: `3=airTemp, 6=airHum, 14=soilMoisture, 3=soilTemp, 4=battery, 15=signal`

---

## Carpeta DebugSensor

Sketch independiente para **desarrollo y diagnóstico** en campo. No usa deep sleep; corre continuamente leyendo sensores cada 5 segundos.

Diferencias importantes respecto al firmware principal:
- Ya tiene configuración para **2 sensores Modbus** (IDs 1 y 2)
- Lee y muestra los 3 valores del sensor: Humedad, Temperatura y **Electroconductividad (EC)**
- Permite detener el ciclo con un botón (pin 0)
- La batería se lee **antes** de encender el VRM (fix de un bug de hardware conocido)

---

## Propuesta de Cambios Futuros

### 1. Cambio de Variable: Humedad Suelo → Electroconductividad

En `sensors.ino`, el registro `data[2]` (EC) ya se lee pero se descarta. El cambio implica:
- Reemplazar `soilMoisture` por `soilConductivity` (o agregar ambas)
- Actualizar el CSV en `datalogger.ino`
- Actualizar el array de IDs de variables en `modem.ino`

### 2. Soporte para 2 Sensores de Suelo

El `DebugSensor.ino` ya muestra cómo hacer esto con `node1` (ID Modbus 1) y `node2` (ID Modbus 2). Para el firmware principal:
- Agregar `ModbusMaster soilSensor2;`
- Agregar variables `soilTemp2`, `soilMoisture2`, `soilEC2`
- Duplicar variables en `idsSensores[]` y en el CSV

### 3. Nuevo Esquema de IDs de Estaciones

Propuesta para hacer la asignación de IDs más legible y extensible:

```cpp
// Estructura propuesta
struct StationConfig {
  int stationId;
  const char* name;
  int sensorIds[8];    // ampliado para 2 sensores de suelo
};

// Configuraciones
const StationConfig stations[] = {
  {7,  "SOIL-01", {76, 76, 81, 81, 86, 91, XX, XX}},
  {8,  "SOIL-02", {77, 77, 82, 82, 87, 92, XX, XX}},
  // nuevas estaciones...
};
```

Esto permite agregar nuevas estaciones sin modificar la cadena `#if / #elif`, y tener un nombre descriptivo por estación.

---

## Descargo de Responsabilidad

Este código se ofrece **"tal cual"**, sin garantías de ningún tipo, expresas o implícitas. El autor no se hace responsable de daños directos, indirectos, incidentales o de cualquier otro tipo derivados del uso de este software. **El usuario lo utiliza bajo su propio riesgo.**
