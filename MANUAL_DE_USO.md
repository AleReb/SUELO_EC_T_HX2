# Manual de Uso - Estación de Monitoreo de Suelo DICTUC

**Versión de Firmware:** 0.3  
**Contacto:** arebolledo@udd.cl (Alejandro Rebolledo)  

Este documento proporciona las instrucciones necesarias para la instalación, configuración y operación diaria de la Estación de Monitoreo de Suelo (Modelo SUELO_EC_T_HX2).

---

## 1. Requisitos Previos y Preparación

Antes de encender o instalar la estación en terreno, asegúrese de contar con lo siguiente:

1. **Tarjeta SD (MicroSD):** Debe estar formateada en **FAT32**. Es obligatoria para el funcionamiento del sistema, ya que aquí se guardarán los respaldos de los datos y la caché de transmisión.
2. **Tarjeta SIM (Micro/Nano SIM según el módem):** Debe contar con saldo/plan de datos activo. El APN preconfigurado en el código actual es `gigsky-02`. Si utiliza otro proveedor de SIM, este parámetro debe ser modificado y reprogramado en el archivo `modem.ino`.
3. **Pilas / Batería:** Asegúrese de que el banco de baterías esté cargado y correctamente conectado al pin de medición (Pin 33) y alimentación principal.
4. **Sensores Conectados:**
   - 1 Sensor DHT21 (Aire)
   - 1 a 2 Sensores de Suelo RS485 (Modbus ID 1 y Modbus ID 2).

## 2. Puesta en Marcha (Primer Encendido)

1. Inserte la tarjeta SD y la tarjeta SIM en sus ranuras correspondientes.
2. Conecte la fuente de alimentación (baterías).
3. Automáticamente, el equipo detectará que es su **Primer Encendido** (`First Boot`).
4. **Secuencia de Iniciación:**
   - Prenderá los periféricos y hará un escaneo interno (I2C, RTC, SD).
   - Los LEDs parpadearán en secuencia indicando el inicio.
   - Creará o verificará los archivos `/DATA[ID].csv` (histórico) y `/cache.csv` (datos por enviar) en la tarjeta SD.
   - Tomará la **primera lectura de todos los sensores**.
   - Intentará conectarse a la red GSM/GPRS para enviar este primer dato al servidor.
5. Luego de este ciclo, el dispositivo entrará en **Deep Sleep (Suspensión Profunda)** para ahorrar energía.

## 3. Funcionamiento Normal en Terreno

Una vez encendida, la estación funciona de forma totalmente autónoma. 
El equipo se despierta brevemente cada **10 segundos**. Al llegar al ciclo número 180 (es decir, cada **30 minutos**), el dispositivo hará lo siguiente de manera automática:

1. Enciende energéticamente los sensores y los lee (Temperatura/Humedad de aire y Temperatura/Humedad/EC de los dos sensores de suelo).
2. Lee el nivel de la batería.
3. Almacena la lectura localmente en la tarjeta SD (`DATA[ID].csv` y `cache.csv`).
4. Enciende el módem celular y **transmite todos los datos pendientes** en el `cache.csv` hacia el servidor.
5. Si el envío es exitoso, los datos se borran del `cache.csv` (pero quedan siempre respaldados en `DATA[ID].csv`).
6. Vuelve a dormir por otros 30 minutos.

## 4. Indicadores Visuales (LEDs)

La placa cuenta con un sistema de LEDs (controlados vía expansor I2C PCF8574) para informar visualmente qué está ocurriendo sin necesidad de conectar un computador.

* 🟡 **LED Amarillo Encendido:** El sistema está ejecutando una acción que consume tiempo, por lo general **transmitiendo datos vía GPRS/Celular** o está en Modo Debug.
* 🟢 **LED Verde Parpadeando:** La acción fue **exitosa** (por ejemplo, los sensores se leyeron correctamente y los datos se guardaron/enviaron sin problemas).
* 🔴 **LED Rojo Encendido/Parpadeando:** Ha ocurrido un **error**. Causas comunes:
  * No hay tarjeta SD insertada o está corrupta.
  * No hay señal celular o no se pudo establecer comunicación con el servidor HTTP.
  * Falla al crear un archivo en la memoria.

## 5. Modo "Debug" (Ráfaga de Muestreo Rápida)

Para facilitar la verificación y calibración en terreno sin necesidad de esperar 30 minutos entre cada lectura, el equipo cuenta con un **Modo Debug de 5 minutos**.

**Para activarlo:**
1. Mantenga presionado el botón de **BOOT** (botón físico asociado al GPIO 0 de la placa ESP32) justo cuando la placa esté en su momento de despertar de 10 segundos.
2. El LED **Amarillo** se encenderá indicando que se entró en Modo Debug.
3. El equipo comenzará a leer todos los sensores **cada 30 segundos** durante un total de **5 minutos** (10 lecturas).
4. Todas estas lecturas serán **guardadas en la memoria SD**.
5. *Nota:* Durante este modo intencionalmente **no se usa la transmisión celular** para ahorrar energía y centrarse en la lectura rápida de diagnóstico.
6. Al finalizar los 5 minutos, el LED Verde parpadeará varias veces y el equipo volverá a su ciclo de sueño normal.

## 6. Extracción de Datos Manual

Si requiere obtener los datos de forma manual, simplemente extraiga la tarjeta MicroSD y léala en un computador. 

* **Archivo de interés:** `/DATA[ID].csv` (ejemplo: `/DATA9.csv`)
* **Formato de los datos:**
  El archivo presenta valores separados por comas:
  `FechaHora, Batería (V), TempAire (°C), HumAire (%), EC_Suelo_1 (µS/cm), TempSuelo_1 (°C), HumSuelo_1 (%), EC_Suelo_2 (µS/cm), TempSuelo_2 (°C), HumSuelo_2 (%)`

---

## Licencia

Este proyecto se encuentra bajo la licencia **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**. 
Para ver una copia de esta licencia, visite [http://creativecommons.org/licenses/by-nc/4.0/](http://creativecommons.org/licenses/by-nc/4.0/) o revise el archivo `LICENSE` adjunto.

## Descargo de Responsabilidad

Este código y su hardware asociado se ofrecen **"tal cual"**, sin garantías de ningún tipo, expresas o implícitas. El autor no se hace responsable de daños directos, indirectos, incidentales o de cualquier otro tipo derivados del uso de este software o hardware. **El usuario lo utiliza bajo su propio riesgo.**
