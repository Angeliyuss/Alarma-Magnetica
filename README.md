# Sistema de Alarma Magnética Antirrobo IoT

Sistema IoT que detecta la apertura no autorizada de un acceso (puerta, cajón,
gabinete o ventanas) mediante un sensor magnético, emite una alarma sonora y visual,
y registra cada apertura con fecha y hora en la nube para su posterior análisis.

---

## Descripción

El dispositivo usa un sensor reed (KY-025) y un imán: cuando el imán se separa del
sensor (acceso abierto) estando el sistema armado, se dispara una alarma (buzzer +
LED rojo) y se registra el evento. Cada apertura se guarda de forma persistente en
Google Sheets y se visualiza en un dashboard web que muestra el historial y la hora
del día en que más se abre el acceso.

**Ciclo IoT completo:** captura (sensor) → almacenamiento (Google Sheets) →
visualización (dashboard) → información para decisiones (hora pico de aperturas).

---

## Integrantes

- Angelo Beconcini
- Sergio Contreras
- Sebastián Bustos
- Benjamín Quijada

---

## Problema que resuelve (Avance 1)

Los accesos a objetos de valor (cajones, gabinetes, vitrinas) no cuentan con un
sistema económico que avise y registre cuándo fueron abiertos. Este proyecto entrega
una alarma de bajo costo que no solo avisa en el momento, sino que **registra el
histórico de aperturas**, permitiendo identificar patrones (por ejemplo, a qué hora
del día ocurren más aperturas) para tomar decisiones de seguridad.

---

## Componentes para replicar el hardware

- ESP32-S3 N16R8
- Sensor magnético reed KY-025
- Buzzer KY-006
- LED RGB KY-016
- Switch deslizante de 3 patas
- Battery Shield 18650 + batería 18650
- Protoboard y cables dupont
- Imán

(Ver detalle de costos en `hardware/BOM.xlsx`)

---

## Instrucciones de instalación

### Firmware
1. Instalar Arduino IDE y el soporte para placas ESP32 (`esp32 by Espressif`).
2. Seleccionar la placa **ESP32S3 Dev Module**.
3. Abrir `firmware/alarma_antirrobo.ino`.
4. Editar las líneas de WiFi con la red propia:
   ```cpp
   const char* ssid     = "TU_RED";
   const char* password = "TU_CLAVE";
   ```
5. Pegar la URL del Google Apps Script en la variable `URL_SHEETS`.
6. Subir el código al ESP32 con el cable USB-C.

### Repositorio de datos (Google Sheets)
1. Crear una planilla con columnas: Fecha | Hora | Timestamp.
2. En Extensiones → Apps Script, pegar el script de `firmware/google_apps_script.js`.
3. Implementar como Aplicación web con acceso "Cualquier persona".
4. Copiar la URL `/exec` y pegarla en el firmware y en el dashboard.

### Dashboard
- Versión online: **https://angeliyuss.github.io/Alarma-Magnetica/**
- O abrir `firmware/index.html` (requiere la URL del script configurada).

---

## Dashboard en vivo

**https://angeliyuss.github.io/Alarma-Magnetica/**

Muestra: aperturas de hoy, total histórico, hora más activa, última apertura,
gráfico de aperturas por hora y tabla de historial completo. Se actualiza
automáticamente cada 30 segundos.

---

## Estructura del repositorio

```
Alarma-Magnetica/
├── README.md
├── FUENTES.md
├── index.html              # Dashboard (servido por GitHub Pages)
├── firmware/
│   ├── alarma_antirrobo.ino
│   └── google_apps_script.js
├── hardware/
│   ├── esquematico.pdf
│   └── BOM.xlsx
├── diseno-3d/
│   ├── encapsulado.f3d
│   ├── planos.pdf
│   └── renders/
├── testing/
│   └── protocolo_pruebas.pdf
└── docs/
    └── reporte_final.pdf
```
