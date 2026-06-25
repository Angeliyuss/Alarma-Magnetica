# Declaración de Fuentes e Integridad Académica

Proyecto: **Sistema de Alarma Magnética Antirrobo IoT**
Asignatura: TEI201 — Taller de Diseño en Ingeniería · UAI
Avance #3 — 25 de junio de 2026

Este documento declara todas las fuentes externas, librerías y usos de inteligencia
artificial empleados en el desarrollo del proyecto, en cumplimiento del código de
honor de la Universidad Adolfo Ibáñez.

---

## 1. Librerías utilizadas

Todas las librerías son parte del core oficial de ESP32 para Arduino (no requieren
instalación externa) y son de código abierto.

| Librería | Versión | Uso en el proyecto | Fuente |
|---------------|---------|----------------------------------------------------|--------|
| WiFi.h | Core ESP32 3.x | Conexión del ESP32-S3 a la red WiFi (hotspot) en modo estación | https://github.com/espressif/arduino-esp32 |
| WebServer.h | Core ESP32 3.x | Servidor web embebido que sirve la página de registro local en la IP del ESP32 | https://github.com/espressif/arduino-esp32 |
| HTTPClient.h | Core ESP32 3.x | Envío de los registros de apertura por HTTP GET hacia Google Sheets | https://github.com/espressif/arduino-esp32 |
| time.h | C estándar / ESP32 | Obtención y formateo de la hora real vía NTP | Biblioteca estándar de C |

---

## 2. Servicios y plataformas externas

| Servicio | Uso en el proyecto | Fuente |
|------------------------|------------------------------------------------------------|--------|
| Google Apps Script | Script `doGet()` que recibe los datos del ESP32 y los guarda en la planilla; también entrega los datos en formato JSON para el dashboard | https://developers.google.com/apps-script |
| Google Sheets | Repositorio de datos persistente (base de datos en la nube) donde se almacena el histórico de aperturas | https://www.google.com/sheets/about/ |
| GitHub Pages | Hosting del dashboard web público | https://pages.github.com/ |
| Servidores NTP | Sincronización de hora real (pool.ntp.org, time.google.com, time.nist.gov) | https://www.pool.ntp.org/ |

---

## 3. Código externo adaptado

No se copiaron fragmentos de código de terceros (Stack Overflow, tutoriales, repos
externos). La estructura del firmware y del dashboard fue desarrollada para este
proyecto con asistencia de IA (ver sección 4), partiendo de la documentación oficial
de las librerías listadas arriba.

---

## 4. Uso de Inteligencia Artificial

Se utilizó **Claude (Anthropic)** como herramienta de asistencia durante el
desarrollo. A continuación se declara qué generó la IA, qué hace ese código y cómo
el equipo lo comprende y adaptó.

### 4.1 Firmware del ESP32 (archivo `alarma_antirrobo.ino`)
- **Herramienta:** Claude (junio 2026)
- **Uso:** Generación de la estructura base del firmware: lectura del sensor KY-025,
  control del buzzer y LED RGB, lógica de armado/desarmado con el switch, servidor
  web y envío a Google Sheets.
- **Adaptación realizada por el equipo:**
  - Se ajustó el nivel de detección del imán (`IMAN_PRESENTE_NIVEL`) tras probar
    físicamente que el sensor entregaba la señal invertida.
  - Se ajustó la zona horaria a UTC-4 (horario de invierno de Chile) aplicando el
    offset manualmente sobre la hora UTC.
  - Se movió el envío a Google Sheets a un **segundo núcleo** del ESP32
    (`xTaskCreatePinnedToCore`) tras detectar que el envío bloqueaba la respuesta
    de la alarma. Esta fue una decisión de diseño del equipo a partir del problema
    observado en pruebas.
- **Comprensión del equipo:** Entendemos que el sensor reed (KY-025) abre/cierra un
  contacto según la presencia del imán; que el pull-up interno mantiene el pin en
  HIGH y el switch lo lleva a LOW; que el ESP32 tiene dos núcleos y por eso la tarea
  de red corre en paralelo sin congelar la alarma; y que la hora se obtiene en UTC y
  se le resta el offset de Chile para mostrarla localmente.

### 4.2 Script de Google Apps Script (`doGet`)
- **Herramienta:** Claude (junio 2026)
- **Uso:** Función que recibe los parámetros enviados por el ESP32 y agrega una fila
  a la planilla; y que, cuando se la consulta sin parámetros, devuelve todo el
  histórico en formato JSON para el dashboard.
- **Comprensión del equipo:** Entendemos que `doGet(e)` se ejecuta cuando se accede a
  la URL del script; que `e.parameter` contiene los datos de la petición; que
  `appendRow()` agrega la fila; y que la respuesta JSON es lo que lee el dashboard.

### 4.3 Dashboard web (`index.html`)
- **Herramienta:** Claude (junio 2026)
- **Uso:** Generación de la página HTML/CSS/JavaScript que lee los datos desde Google
  Sheets, calcula estadísticas (aperturas del día, total, hora pico) y dibuja el
  gráfico de barras de aperturas por hora.
- **Adaptación realizada por el equipo:** Se decidió usar la columna de *timestamp*
  como fuente de verdad para calcular fecha y hora, porque Google Sheets reinterpretó
  el texto de fecha/hora en otro formato. El cálculo de la "hora más activa" se basa
  en agrupar todos los registros por hora del día.
- **Comprensión del equipo:** Entendemos que el dashboard hace `fetch()` a la URL del
  script, recibe el JSON, recorre los registros, los agrupa por hora y genera las
  barras proporcionalmente al conteo máximo.

---

## 5. Declaración final

El equipo declara que comprende el funcionamiento de todo el código presentado,
independientemente de si fue escrito directamente o generado con asistencia de IA, y
está en condiciones de explicar cualquier función durante la evaluación.
