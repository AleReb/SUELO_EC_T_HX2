# Manual de Uso en Terreno
## Estacion de Monitoreo de Suelo

Este equipo mide condiciones del suelo y del ambiente, guarda los datos en una tarjeta SD y los envia automaticamente por red celular cuando hay cobertura.

El sistema esta pensado para operar de forma autonoma en terreno, con ciclos de medicion y suspension para reducir el consumo de energia.

---

## 1. Que mide la estacion

La estacion registra automaticamente:

- Temperatura del aire
- Humedad del aire
- Temperatura del suelo
- Humedad del suelo
- Conductividad electrica del suelo
- Nivel de senal celular
- Voltaje de bateria

Si no hay cobertura celular, los datos no se pierden: quedan almacenados localmente y se envian cuando la comunicacion se recupera.

---

## 2. Antes de instalar en terreno

Verifique siempre lo siguiente:

- La tarjeta SD esta insertada y operativa
- La SIM tiene plan de datos activo
- La bateria esta conectada o cargada
- Los sensores estan conectados
- La antena celular esta conectada

Si falta cualquiera de estos elementos, el sistema puede medir parcialmente o dejar de transmitir.

---

## 3. Funcionamiento normal

En operacion normal, el equipo:

1. Despierta automaticamente
2. Lee los sensores
3. Guarda la medicion en la SD
4. Intenta transmitir los datos
5. Apaga perifericos y entra en suspension

Este ciclo se repite cada **30 minutos**.

No requiere intervencion humana durante la operacion normal.

---

## 4. Indicadores luminosos

Las luces del equipo permiten diagnosticar rapidamente el estado de la estacion en terreno.

### Luz verde

<div align="center">
  <img src="https://raw.githubusercontent.com/AleReb/SUELO_EC_T_HX2/main/luzverde.png" alt="Luz verde" width="220" />
</div>

Indica:

- Medicion correcta
- Guardado correcto en la tarjeta SD
- Fin exitoso del ciclo de operacion

Si aparece de forma periodica, el equipo esta funcionando correctamente.

---

### Luz amarilla

<div align="center">
  <img src="https://raw.githubusercontent.com/AleReb/SUELO_EC_T_HX2/main/luzamarilla.png" alt="Luz amarilla" width="220" />
</div>

Indica:

- Transmision de datos en curso
- Modo burst activo
- Equipo ocupado en una tarea de mayor duracion

Si aparece durante algunos segundos, es normal.

---

### Luz roja

Indica una condicion de error, normalmente asociada a almacenamiento o comunicacion.

Si aparece de forma persistente, revisar:

- Tarjeta SD
- Senal celular
- Alimentacion del equipo
- Estado general de conexiones

---

## 5. Que pasa si no hay senal celular

No es una falla critica.

En esa condicion, el equipo:

- Guarda todos los datos internamente
- Mantiene un cache de envio pendiente
- Reintenta la transmision en ciclos posteriores

No es necesario intervenir de inmediato si la estacion sigue midiendo y almacenando.

---

## 6. Modo de medicion intensiva (modo burst)

El equipo dispone de un modo especial para medicion intensiva.

Este modo sirve para:

- Diagnostico en terreno
- Validacion tecnica
- Campanas de observacion de alta frecuencia

### Como activarlo

1. Energizar o despertar el equipo
2. Presionar el boton `BOOT` en el momento de arranque o wakeup
3. Verificar que el equipo entre al modo burst

### Que hace el modo burst

- Realiza 36 lecturas
- Guarda una lectura cada 5 minutos
- Tiene una duracion total aproximada de 3 horas
- Almacena los datos en la SD
- Omite la transmision normal mientras el burst esta en ejecucion

Durante este modo, la luz amarilla actua como indicador de actividad.

---

## 7. Almacenamiento de datos

Los datos se guardan en la tarjeta SD en dos archivos principales:

- `DATA[ID].CSV`: historial permanente de mediciones
- `cache.csv`: datos pendientes de transmision

Esto permite recuperar informacion incluso si:

- No hubo senal durante varios dias
- El modem no pudo transmitir
- Hubo reinicios o fallas de red

---

## 8. Consumo energetico

El sistema esta disenado para bajo consumo.

Por eso:

- Permanece dormido la mayor parte del tiempo
- Solo activa sensores, SD y modem cuando corresponde
- Apaga perifericos antes de volver a suspension

Esto es normal y forma parte del diseno del equipo.

---

## 9. Cuando intervenir en terreno

Se recomienda revisar el equipo si ocurre alguna de estas condiciones:

- No hay luces durante varias horas
- La luz roja permanece encendida o aparece en todos los ciclos
- No hay datos reportados durante varios dias
- El equipo sufrio golpes, ingreso de agua o humedad extrema
- La antena o el cableado presentan danos visibles

---

## 10. Recomendaciones de operacion

Para una operacion confiable en terreno:

- Mantenga la SD correctamente insertada
- Verifique periodicamente el estado de la bateria
- Evite desconectar antena o sensores con el equipo energizado
- Revise la cobertura celular del sitio de instalacion
- Inspeccione sellos, conectores y caja ante condiciones climaticas severas

---

## 11. Responsabilidad operativa

El correcto funcionamiento depende de:

- Instalacion adecuada
- Estado de la bateria
- Cobertura celular
- Integridad de sensores, antena y tarjeta SD
- Condiciones ambientales del sitio

El sistema es autonomo, pero requiere supervision periodica.

---

## 12. Nota para exportacion a PDF

Este archivo fue ordenado para convertirlo con `pandoc`. Las imagenes usan URLs `raw.githubusercontent.com`, que funcionan mejor que los enlaces `blob` de GitHub al exportar a HTML o PDF.
