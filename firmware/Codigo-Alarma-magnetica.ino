// ===== ALARMA ANTIRROBO + REGISTRO WEB + GOOGLE SHEETS - ESP32-S3 =====
// El envio a Google Sheets corre en el SEGUNDO NUCLEO para no trabar la alarma.
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <time.h>

// ---------- PINES ----------
#define PIN_SENSOR  4
#define PIN_BUZZER  5
#define PIN_SWITCH  6
#define PIN_LED_R   7
#define PIN_LED_G   8
#define PIN_LED_B   9

// ---------- AJUSTE: si el iman queda invertido, cambia HIGH por LOW ----------
#define IMAN_PRESENTE_NIVEL  HIGH

// ---------- WiFi ----------
const char* ssid     = "iPhone Angeliyu";
const char* password = "21012101Ca";

// ---------- Google Sheets ----------
const char* URL_SHEETS = "https://script.google.com/macros/s/AKfycbze7cZP5LEMvWSCMegIvz0YPmM0B73EwZMt4l9lqQPetthbR3F0ZVaq6-07lGdCMdi1kQ/exec";

const long OFFSET_CHILE = -4 * 3600;

WebServer server(80);

const int MAX_EVENTOS = 200;
time_t eventos[MAX_EVENTOS];
int totalEventos = 0;
bool alarmaPrevia = false;

// Cola para el segundo nucleo
volatile time_t colaEnvio = 0;
volatile bool hayEnvioPendiente = false;

void setLED(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r);
  digitalWrite(PIN_LED_G, g);
  digitalWrite(PIN_LED_B, b);
}

String soloFecha(time_t t) {
  time_t local = t + OFFSET_CHILE;
  struct tm tm_info;
  gmtime_r(&local, &tm_info);
  char buf[20];
  strftime(buf, sizeof(buf), "%d/%m/%Y", &tm_info);
  return String(buf);
}

String soloHora(time_t t) {
  time_t local = t + OFFSET_CHILE;
  struct tm tm_info;
  gmtime_r(&local, &tm_info);
  char buf[20];
  strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
  return String(buf);
}

String formatearHora(time_t t) {
  return soloFecha(t) + "  " + soloHora(t);
}

// ---- Esta tarea corre en el NUCLEO 0, separada de la alarma ----
void tareaSheets(void* parametro) {
  for (;;) {
    if (hayEnvioPendiente && WiFi.status() == WL_CONNECTED) {
      time_t t = colaEnvio;
      HTTPClient http;
      String url = String(URL_SHEETS) + "?fecha=" + soloFecha(t) +
                   "&hora=" + soloHora(t) + "&timestamp=" + String((unsigned long)t);
      url.replace(" ", "%20");
      http.begin(url);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      int codigo = http.GET();
      Serial.print("Enviado a Sheets, codigo HTTP: ");
      Serial.println(codigo);
      http.end();
      hayEnvioPendiente = false;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void registrarEvento() {
  if (totalEventos < MAX_EVENTOS) {
    time_t ahora;
    time(&ahora);
    eventos[totalEventos] = ahora;
    totalEventos++;
    colaEnvio = ahora;
    hayEnvioPendiente = true;
  }
}

void manejarRaiz() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>";
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
    html += "<table><tr><th>#</th><th>Fecha y hora de apertura</th></tr>";
    for (int i = totalEventos - 1; i >= 0; i--) {
      html += "<tr><td>" + String(i + 1) + "</td><td>" + formatearHora(eventos[i]) + "</td></tr>";
    }
    html += "</table>";
  }
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  noTone(PIN_BUZZER);
  setLED(1, 0, 0);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Abre esta IP en el navegador: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

  Serial.print("Sincronizando hora");
  time_t ahora = 0;
  int intentos = 0;
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

  server.on("/", manejarRaiz);
  server.begin();

  xTaskCreatePinnedToCore(tareaSheets, "tareaSheets", 8192, NULL, 1, NULL, 0);
}

void loop() {
  server.handleClient();

  bool armado = (digitalRead(PIN_SWITCH) == LOW);

  if (armado) {
    bool imanPresente = (digitalRead(PIN_SENSOR) == IMAN_PRESENTE_NIVEL);

    if (imanPresente) {
      setLED(0, 1, 0);
      noTone(PIN_BUZZER);
      alarmaPrevia = false;
    } else {
      setLED(1, 0, 0);
      tone(PIN_BUZZER, 2000);
      if (!alarmaPrevia) {
        registrarEvento();
        alarmaPrevia = true;
      }
    }
  } else {
    noTone(PIN_BUZZER);
    setLED(1, 0, 0);
    alarmaPrevia = false;
  }

  delay(50);
}