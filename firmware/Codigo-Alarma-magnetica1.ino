/* ============================================================================
   SISTEMA DE ALARMA MAGNÉTICA ANTIRROBO IoT
   TEI201 - Taller de Diseño en Ingeniería - UAI - Avance #3
   ----------------------------------------------------------------------------
   Microcontrolador: ESP32-S3 N16R8

   Qué hace:
   - Detecta la apertura de un acceso mediante un sensor magnético (reed KY-025).
   - Si el sistema está armado y se aleja el imán, dispara una alarma (buzzer
     + LED rojo) y registra el evento con fecha y hora real.
   - Cada apertura se guarda de forma persistente en Google Sheets (nube).
   - Sirve una página web local con el historial de aperturas.

   Detalle clave: el envío a Google Sheets se ejecuta en el SEGUNDO NÚCLEO del
   ESP32, para que la alarma reaccione al instante y no se congele mientras
   se comunica con internet.
   ========================================================================== */

#include <WiFi.h>         // Conexión WiFi
#include <WebServer.h>    // Servidor web embebido (página local)
#include <HTTPClient.h>   // Cliente HTTP para enviar datos a Google Sheets
#include <time.h>         // Manejo de hora real (NTP)

// ---------- PINES (conexión física de cada componente al ESP32) ----------
#define PIN_SENSOR  4   // Salida digital (DO) del sensor magnético KY-025
#define PIN_BUZZER  5   // Señal del buzzer KY-006
#define PIN_SWITCH  6   // Switch de armar / desarmar el sistema
#define PIN_LED_R   7   // Canal rojo del LED RGB
#define PIN_LED_G   8   // Canal verde del LED RGB
#define PIN_LED_B   9   // Canal azul del LED RGB

// El sensor puede entregar la señal invertida según el módulo. Este parámetro
// permite ajustar a qué nivel lógico se considera "imán presente" sin recablear.
#define IMAN_PRESENTE_NIVEL  HIGH

// ---------- Credenciales de la red WiFi ----------
const char* ssid     = "iPhone Angeliyu";
const char* password = "21012101Ca";

// ---------- URL del Google Apps Script (backend de la planilla) ----------
const char* URL_SHEETS = "https://script.google.com/macros/s/AKfycbze7cZP5LEMvWSCMegIvz0YPmM0B73EwZMt4l9lqQPetthbR3F0ZVaq6-07lGdCMdi1kQ/exec";

// Offset horario de Chile en segundos. Invierno = UTC-4. En verano cambiar a -3.
const long OFFSET_CHILE = -4 * 3600;

WebServer server(80);   // Servidor web en el puerto 80

// ---------- Almacenamiento local de eventos ----------
const int MAX_EVENTOS = 200;        // Capacidad máxima de registros en memoria
time_t eventos[MAX_EVENTOS];        // Arreglo con la hora (timestamp) de cada apertura
int totalEventos = 0;               // Cantidad de aperturas registradas
bool alarmaPrevia = false;          // Evita registrar la misma apertura varias veces

// ---------- Variables compartidas con el segundo núcleo ----------
// 'volatile' indica que pueden cambiar desde otra tarea/núcleo en cualquier momento.
volatile time_t colaEnvio = 0;          // Evento pendiente de enviar a Sheets
volatile bool hayEnvioPendiente = false; // Indica si hay algo por enviar

// Enciende el LED RGB con la combinación de colores indicada (1 = encendido).
void setLED(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r);
  digitalWrite(PIN_LED_G, g);
  digitalWrite(PIN_LED_B, b);
}

// Convierte un timestamp UTC a la fecha local de Chile (dd/mm/aaaa).
String soloFecha(time_t t) {
  time_t local = t + OFFSET_CHILE;   // Aplica el offset de Chile
  struct tm tm_info;
  gmtime_r(&local, &tm_info);        // gmtime para NO volver a aplicar zona horaria
  char buf[20];
  strftime(buf, sizeof(buf), "%d/%m/%Y", &tm_info);
  return String(buf);
}

// Convierte un timestamp UTC a la hora local de Chile (HH:MM:SS).
String soloHora(time_t t) {
  time_t local = t + OFFSET_CHILE;
  struct tm tm_info;
  gmtime_r(&local, &tm_info);
  char buf[20];
  strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
  return String(buf);
}

// Devuelve fecha y hora juntas, para mostrar en la página web.
String formatearHora(time_t t) {
  return soloFecha(t) + "  " + soloHora(t);
}

/* ---- TAREA DEL SEGUNDO NÚCLEO ----
   Corre en paralelo a la alarma. Revisa si hay un evento pendiente y, si hay
   conexión WiFi, lo envía a Google Sheets. Así la comunicación con internet
   nunca bloquea la detección de la alarma. */
void tareaSheets(void* parametro) {
  for (;;) {   // Bucle infinito propio de esta tarea
    if (hayEnvioPendiente && WiFi.status() == WL_CONNECTED) {
      time_t t = colaEnvio;
      HTTPClient http;
      // Arma la URL con los datos del evento como parámetros
      String url = String(URL_SHEETS) + "?fecha=" + soloFecha(t) +
                   "&hora=" + soloHora(t) + "&timestamp=" + String((unsigned long)t);
      url.replace(" ", "%20");   // Codifica los espacios para la URL
      http.begin(url);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Google redirige; hay que seguirlo
      int codigo = http.GET();   // Realiza la petición
      Serial.print("Enviado a Sheets, codigo HTTP: ");
      Serial.println(codigo);    // 200 = enviado con éxito
      http.end();
      hayEnvioPendiente = false; // Marca como enviado
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // Cede tiempo de CPU (no bloquea)
  }
}

// Registra una nueva apertura: la guarda en memoria y la deja en cola para Sheets.
void registrarEvento() {
  if (totalEventos < MAX_EVENTOS) {
    time_t ahora;
    time(&ahora);                 // Hora actual (UTC)
    eventos[totalEventos] = ahora;
    totalEventos++;
    colaEnvio = ahora;            // Deja el evento para que el otro núcleo lo envíe
    hayEnvioPendiente = true;
  }
}

// Construye y entrega la página web con la tabla de aperturas registradas.
void manejarRaiz() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";  // Se actualiza sola cada 5 s
  html += "<title>Registro de Aperturas</title>";
  html += "<style>body{font-family:sans-serif;background:#111;color:#eee;padding:20px;}";
  html += "h1{color:#e24b4a;}table{width:100%;border-collapse:collapse;margin-top:15px;}";
  html += "th,td{border:1px solid #444;padding:10px;text-align:left;}th{background:#222;}";
  html += ".vacio{color:#888;margin-top:20px;}</style></head><body>";
  html += "<h1>Registro de Aperturas</h1>";
  html += "<p>Total de aperturas: <b>" + String(totalEventos) + "</b></p>";

  if (totalEventos == 0) {
    html += "<p class='vacio'>Sin aperturas registradas aun.</p>";
  } else {
    // Recorre los eventos del más reciente al más antiguo
    html += "<table><tr><th>#</th><th>Fecha y hora de apertura</th></tr>";
    for (int i = totalEventos - 1; i >= 0; i--) {
      html += "<tr><td>" + String(i + 1) + "</td><td>" + formatearHora(eventos[i]) + "</td></tr>";
    }
    html += "</table>";
  }
  html += "</body></html>";
  server.send(200, "text/html", html);   // Envía la página al navegador
}

void setup() {
  Serial.begin(115200);   // Inicia comunicación serial para depuración

  // Configura los pines de entrada (con resistencia pull-up interna) y salida
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  noTone(PIN_BUZZER);   // Buzzer apagado al iniciar
  setLED(1, 0, 0);      // LED en rojo al iniciar (sistema en reposo)

  // ---- Conexión a la red WiFi ----
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {  // Espera hasta conectar
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Abre esta IP en el navegador: ");
  Serial.println(WiFi.localIP());   // Muestra la IP para acceder a la página

  // ---- Sincronización de la hora real por NTP ----
  // Se traen 3 servidores de respaldo por si alguno no responde.
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

  Serial.print("Sincronizando hora");
  time_t ahora = 0;
  int intentos = 0;
  // Espera hasta tener una hora válida (o se rinde tras 30 intentos)
  while (ahora < 100000 && intentos < 30) {
    time(&ahora);
    Serial.print(".");
    delay(500);
    intentos++;
  }
  Serial.println();
  if (ahora > 100000) {
    Serial.print("Hora sincronizada (Chile): ");
    Serial.println(formatearHora(ahora));
  } else {
    Serial.println("NO se pudo sincronizar (revisa internet del hotspot)");
  }

  // ---- Inicia el servidor web ----
  server.on("/", manejarRaiz);   // Ruta principal -> muestra la tabla
  server.begin();

  // ---- Lanza la tarea de envío a Sheets en el núcleo 0 ----
  // La alarma corre en el núcleo 1; el envío a internet en el 0, en paralelo.
  xTaskCreatePinnedToCore(tareaSheets, "tareaSheets", 8192, NULL, 1, NULL, 0);
}

void loop() {
  server.handleClient();   // Atiende las peticiones a la página web

  // El sistema está armado cuando el switch conecta el pin a GND (LOW)
  bool armado = (digitalRead(PIN_SWITCH) == LOW);

  if (armado) {
    // Lee si el imán está presente (acceso cerrado) o ausente (abierto)
    bool imanPresente = (digitalRead(PIN_SENSOR) == IMAN_PRESENTE_NIVEL);

    if (imanPresente) {
      // Acceso cerrado: todo en orden
      setLED(0, 1, 0);          // Verde
      noTone(PIN_BUZZER);       // Sin sonido
      alarmaPrevia = false;
    } else {
      // Acceso abierto: ¡alarma!
      setLED(1, 0, 0);          // Rojo
      tone(PIN_BUZZER, 2000);   // Suena a 2 kHz
      if (!alarmaPrevia) {      // Registra solo una vez por apertura
        registrarEvento();
        alarmaPrevia = true;
      }
    }
  } else {
    // Sistema desarmado: todo en reposo
    noTone(PIN_BUZZER);
    setLED(1, 0, 0);            // Rojo (desarmado)
    alarmaPrevia = false;
  }

  delay(50);   // Pequeña pausa para estabilidad
}
