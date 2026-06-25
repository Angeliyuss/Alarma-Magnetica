# Protocolo de Pruebas y Validación

Proyecto: Sistema de Alarma Magnética Antirrobo IoT
TEI201 — Avance #3

---

## Objetivo general

Verificar que el sistema captura de forma confiable las aperturas de un acceso,
las almacena de forma persistente y las visualiza correctamente en el dashboard,
respondiendo al problema de registrar y analizar accesos no autorizados.

---

## Prueba 1 — Detección de apertura (sensor)

**Objetivo:** Verificar que el sensor detecta correctamente la separación del imán.

**Condiciones:** Sistema armado (switch en posición ON), imán en contacto con el sensor.

**Procedimiento:**
1. Armar el sistema con el switch.
2. Verificar que el LED queda en verde (acceso cerrado, imán presente).
3. Separar el imán del sensor.
4. Observar la respuesta del sistema.

**Criterio de éxito:** Al separar el imán, el LED cambia a rojo y el buzzer suena.
Al volver a acercar el imán, el buzzer se detiene y el LED vuelve a verde.

**Resultado:** Cumplido. El sistema responde de forma inmediata.

**Falla detectada y solución:** En las primeras pruebas, la lógica estaba invertida
(el buzzer sonaba con el imán presente). Se identificó que el módulo KY-025 entregaba
la señal digital invertida y se corrigió en el firmware mediante el parámetro
`IMAN_PRESENTE_NIVEL`, que permite ajustar el nivel lógico sin recablear.

---

## Prueba 2 — Sincronización de hora real (NTP)

**Objetivo:** Verificar que cada registro tiene la hora correcta de Chile.

**Condiciones:** ESP32 conectado a WiFi con acceso a internet.

**Procedimiento:**
1. Reiniciar el ESP32 y observar el Serial Monitor.
2. Verificar el mensaje "Hora sincronizada (Chile)".
3. Comparar con la hora real del momento.

**Criterio de éxito:** La hora mostrada coincide con la hora local de Santiago (UTC-4).

**Resultado:** Cumplido tras corrección.

**Falla detectada y solución:** Inicialmente la hora aparecía adelantada y, en otros
intentos, el ESP no lograba sincronizar (se quedaba en "Sincronizando hora..."). Se
detectó que (a) el offset debía aplicarse manualmente sobre UTC para evitar un doble
ajuste, y (b) el hotspot perdía internet intermitentemente. Se agregaron tres
servidores NTP de respaldo y se aplicó el offset de Chile (UTC-4) de forma manual.

---

## Prueba 3 — Almacenamiento persistente (Google Sheets)

**Objetivo:** Verificar que cada apertura se guarda en la nube y persiste tras apagar.

**Condiciones:** Sistema armado, conexión a internet activa.

**Procedimiento:**
1. Generar varias aperturas separando el imán.
2. Verificar en el Serial Monitor el mensaje "Enviado a Sheets, codigo HTTP: 200".
3. Abrir la planilla de Google Sheets y confirmar las filas nuevas.
4. Apagar y reiniciar el ESP32; volver a abrir la planilla.

**Criterio de éxito:** Cada apertura genera una fila con fecha, hora y timestamp;
los datos permanecen tras reiniciar el dispositivo.

**Resultado:** Cumplido. Los datos persisten en la nube.

**Falla detectada y solución:** Al registrar aperturas, la alarma se congelaba 2-3
segundos porque el envío a Google bloqueaba el programa. Se resolvió moviendo el
envío a Google Sheets al **segundo núcleo del ESP32** (procesamiento en paralelo), de
modo que la alarma responde al instante mientras los datos se envían en segundo plano.
Adicionalmente, se observaron errores HTTP -11 cuando el hotspot perdía señal; se
confirmó que al recuperar la conexión los envíos vuelven a completarse con código 200.

---

## Prueba 4 — Visualización en dashboard

**Objetivo:** Verificar que el dashboard muestra los datos y genera información útil.

**Condiciones:** Dashboard publicado en GitHub Pages, con datos en la planilla.

**Procedimiento:**
1. Abrir el dashboard (https://angeliyuss.github.io/Alarma-Magnetica/).
2. Verificar las tarjetas de estadísticas y el gráfico por hora.
3. Generar una apertura nueva y esperar la actualización automática (30 s).

**Criterio de éxito:** El dashboard muestra el histórico, identifica la hora con más
aperturas y se actualiza con los datos nuevos.

**Resultado:** Cumplido.

---

## Análisis de resultados (cuantitativo)

- **Tasa de detección:** 100% de las aperturas (con duración mayor a ~1 segundo)
  fueron detectadas y registradas correctamente.
- **Latencia de respuesta de la alarma:** inmediata (< 0,1 s) tras mover el envío de
  datos al segundo núcleo.
- **Limitación identificada:** aperturas extremadamente rápidas (separar y juntar el
  imán en menos de ~1 segundo, dos veces seguidas) pueden contarse como una sola. Se
  evaluó y se determinó que no es relevante para el caso de uso real, ya que una
  apertura real de un acceso dura más que ese umbral.
- **Confiabilidad de envío:** los envíos fallan (HTTP -11) solo cuando el hotspot
  pierde internet; con conexión estable la tasa de éxito es de código 200.

---

## Validación contra el problema original

El problema planteado es registrar y analizar las aperturas de un acceso para detectar
patrones de uso no autorizado. El sistema cumple este objetivo: durante las pruebas se
acumularon más de 50 registros reales en la planilla, y el dashboard identificó
automáticamente la franja horaria con mayor número de aperturas. Esto demuestra que el
dispositivo no solo avisa en el momento (alarma), sino que **genera información
accionable** (la hora pico de aperturas), permitiendo a un usuario tomar decisiones de
seguridad sobre cuándo reforzar la vigilancia del acceso.
