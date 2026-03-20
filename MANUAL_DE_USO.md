# Manual de Uso - Estacion de Monitoreo de Suelo!

**Firmware revisado:** 0.40  
**Plataforma:** ESP32 (Arduino Framework)  
**Contacto:** arebolledo@udd.cl

Este manual describe la operacion real del firmware actual del proyecto `SUELO_EC_T_HX2`, segun el codigo fuente presente en este repositorio.

---

## 1. Resumen del sistema

La estacion registra y transmite:

- Temperatura y humedad del aire con un sensor DHT21.
- Temperatura, humedad y conductividad electrica de 2 sensores de suelo RS485/Modbus.
- Voltaje de bateria.
- Calidad de senal GSM.

Los datos se guardan en MicroSD y luego se transmiten por red celular al servidor configurado en el firmware.

---

## 2. Requisitos previos

Antes de instalar o energizar la estacion, verifique lo siguiente:

1. MicroSD insertada y funcional.
2. SIM con plan de datos activo.
3. APN compatible con el operador.
4. Bateria o alimentacion conectada.
5. Sensores conectados:
   - 1 x DHT21 en GPIO 14.
   - 2 x sensores de suelo RS485 con direcciones Modbus 1 y 2.

### APN actual

El firmware usa actualmente:

`gigsky-02`

Si la SIM usa otro APN, debe cambiarse en [SUELO_EC_T_HX2.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/SUELO_EC_T_HX2.ino).

---

## 3. Secuencia de arranque

En el primer arranque (`firstBoot`), el equipo realiza esta secuencia:

1. Inicializa la deteccion del boton de modo burst en GPIO 0.
2. Enciende perifericos mediante la VRM.
3. Inicializa I2C.
4. Escanea direcciones I2C.
5. Verifica la tarjeta SD.
6. Crea o valida los archivos `/DATA[ID].CSV` y `/cache.csv`.
7. Verifica el RTC DS3231.
8. Lee sensores.
9. Guarda la lectura en la SD.
10. Intenta transmitir la lectura actual.
11. Apaga perifericos y entra en deep sleep.

---

## 4. Funcionamiento normal

El firmware actual no usa ciclos de 10 segundos acumulados. El flujo real es:

1. El ESP32 despierta.
2. Enciende perifericos.
3. Verifica RTC y hora.
4. Lee sensores.
5. Guarda la lectura en SD.
6. Intenta transmitir por GSM/GPRS.
7. Apaga perifericos.
8. Entra en deep sleep por 30 minutos.

### Tiempo de suspension

El tiempo de sleep configurado es:

`1800000000 us = 1800 s = 30 minutos`

Implementado en [SUELO_EC_T_HX2.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/SUELO_EC_T_HX2.ino).

---

## 5. Modo burst

El firmware actual implementa un debug largo de 3 horas. En su lugar existe un **modo burst** activado con el boton `BOOT` en GPIO 0.

### Como activarlo

- Presione el boton BOOT durante el arranque o provoque un wakeup externo por GPIO 0.
- Si el equipo detecta esta solicitud, suspende la transmision normal y entra en modo burst.

### Que hace el modo burst

- Toma **36 lecturas**.
- Guarda una lectura cada **5 minutos**.
- Duracion total aproximada: **3 horas**.
- Guarda todas las lecturas en la SD.
- No ejecuta `sendData()` durante ese flujo.

### Indicacion visual

- Amarillo: modo burst o transmision en curso se prende cada 10 segundos por 1 segundo.
- Verde: lectura/guardado correcto.
- Rojo: falla visible, normalmente asociada a SD o comunicacion.

La implementacion esta en [sensors.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/sensors.ino).

---

## 6. Sensores y valores registrados

### Aire

- Temperatura de aire.
- Humedad de aire.

### Suelo 1 y Suelo 2

Cada sensor Modbus entrega 3 registros desde `0x0000`:

- Humedad del suelo.
- Temperatura del suelo.
- Conductividad electrica del suelo.

### Bateria

El voltaje se estima desde el ADC en GPIO 33.

---

## 7. Archivos generados en la SD

### Historico

Archivo:

`/DATA[stationId].CSV`

Ejemplo:

`/DATA11.CSV`

Contiene todas las lecturas historicas y no se borra automaticamente.

Cabecera real:

`FechaHora,Bateria,TemperaturaExterna,HumedadExterna,ElectroconductividadSuelo1,TemperaturaSuelo1,HumedadSuelo1,ElectroconductividadSuelo2,TemperaturaSuelo2,HumedadSuelo2`

### Cache de transmision

Archivo:

`/cache.csv`

Contiene lecturas pendientes de envio cuando falla el modem, la red o la peticion HTTP. Si el envio posterior resulta exitoso, el archivo se vacia.

Formato real del cache:

`epoch,bateria,temp_aire,hum_aire,ec1,temp_suelo1,hum_suelo1,ec2,temp_suelo2,hum_suelo2`

La logica de almacenamiento esta en [datalogger.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/datalogger.ino).

---

## 8. Transmision de datos

La transmision se realiza con el modem conectado a `Serial1`:

- RX/TX del modem: GPIO 17 y GPIO 16 segun la configuracion del sketch.
- Velocidad: `115200`.
- APN actual: `gigsky-02`.
- Protocolo: comandos AT con peticion HTTP GET.

### Comportamiento ante falla

- Si el modem no responde, no hay red o falla el envio HTTP, la lectura actual se guarda en `/cache.csv`.
- Si la lectura actual se envia correctamente, el equipo intenta reenviar el contenido pendiente de `/cache.csv`.

La logica principal esta en [modem.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/modem.ino).

---

## 9. Pinout principal

| Pin ESP32 | Funcion |
|-----------|---------|
| 0 | Boton BOOT / solicitud de modo burst |
| 4 | CS de tarjeta SD |
| 13 | VRM enable, activo en LOW |
| 14 | DHT21 |
| 16 | UART modem |
| 17 | UART modem |
| 25 | RS485 RX |
| 26 | RS485 TX |
| 27 | RS485 DE |
| 32 | RS485 RE |
| 33 | ADC bateria |

I2C se usa para:

- RTC DS3231
- Expansor PCF8574 para LEDs

---

## 10. Indicadores visuales

Los LEDs dependen del expansor I2C PCF8574 en direccion `0x20`.

- Verde: operacion correcta, lectura o guardado exitoso.
- Amarillo: transmision o modo burst.
- Rojo: error o fallo de comunicacion.

Las rutinas estan en [i2c.ino](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/i2c.ino).

---

## 11. Observaciones importantes
1. El nombre del archivo historico real usa extension `.CSV` en mayusculas.
2. El `stationId` actual en el codigo es `11`.
3. La version actual definida en codigo es `0.40`.

---

## Licencia

Este proyecto se distribuye bajo licencia **CC BY-NC 4.0**. Revise [LICENSE](/c:/Users/Ale/Downloads/SUELO_EC_T_HX2/LICENSE) para el texto completo.

## Descargo

El software y el hardware asociado se entregan "tal cual", sin garantias explicitas ni implicitas. El uso en terreno y la validacion operativa quedan bajo responsabilidad del usuario.
