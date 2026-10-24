# Keyer Iambico y Decodificador CW (LU2FTI) - ESP32-C3 Mini

Sistema de manipulador telegrafico iambico (Modos A y B) y decodificador CW en tiempo real con pantalla OLED de 1.3 pulgadas (Controlador SH1106), disenado para el microcontrolador ESP32-C3 Mini.

---

## Caracteristicas Principales

* Manipulador Iambico Doble: Soporte seleccionable para Modo A y Modo B.
* Decodificador CW en Tiempo Real: Analiza y muestra los caracteres decodificados directamente en pantalla a partir de la manipulacion.
* Pantalla OLED 1.3" (SH1106): Interfaz visual basada en la libreria U8g2, con visualizacion de texto en vivo y barra de estado.
* Menu Interactivo Integrado: Permite ajustar la velocidad (WPM) y el modo iambico sin reprogramar el chip.
* Memoria No Volatil (EEPROM): Guarda la configuracion de velocidad y modo en la EEPROM interna del ESP32-C3.
* Sidetone y Salida TX: Salida de tono local (sidetone) configurable y linea de salida para activacion de transmisor (Key Out).

---

## Conexiones y Pinout

| Pin en Codigo | GPIO ESP32-C3 | Funcion / Periferico |
| :--- | :---: | :--- |
| `PIN_DIT` | GPIO 2 | Paleta de Punto (Dit) (Pull-up interno) |
| `PIN_DAH` | GPIO 3 | Paleta de Raya (Dah) (Pull-up interno) |
| `PIN_COMMAND` | GPIO 0 | Boton de Comando / Menu (Pull-up interno) |
| `PIN_SIDETONE`| GPIO 1 | Salida de Tono Sidetone (Buzzer / Audio) |
| `PIN_KEY_OUT` | GPIO 6 | Salida de Transmision (Key Out hacia equipo) |
| `I2C_SDA` | GPIO 5 | Linea SDA para Display OLED SH1106 |
| `I2C_SCL` | GPIO 4 | Linea SCL para Display OLED SH1106 |

---

## Librerias Requeridas

Para compilar e instalar este sketch en el ESP32-C3 Mini, se requiere tener instalada la siguiente libreria desde el Administrador de Librerias de Arduino IDE:

* U8g2 (por olikraus): Para el control de la pantalla OLED SH1106 de 128x64 por I2C.
* Nota: `Wire` y `EEPROM` vienen integradas en el nucleo (core) de ESP32 para Arduino.

---

## Guia de Uso y Controles

### 1. Modo Normal (Operacion)
* Manipulacion Iambica: Al presionar las paletas de punto (DIT) y raya (DAH), el sistema genera los elementos telegraficos, decodifica la senal y muestra los caracteres en las lineas superiores de la pantalla.
* Linea de Estado (Inferior): Muestra la velocidad actual en WPM, el modo iambico (IAM-A o IAM-B) y el indicativo de estacion (LU2FTI).
* Boton de Comando (`PIN_COMMAND`):
  * Pulsacion corta (menor a 1 segundo): Limpia el buffer de texto en pantalla y emite un tono corto de confirmacion (800 Hz).
  * Pulsacion larga (mayor a 1 segundo): Ingresa al Menu de Configuracion.

### 2. Menu Interactivo
Al ingresar al menu, la pantalla mostrara las opciones disponibles:
1. Velocidad (WPM): Ajustable de 5 a 50 WPM.
   * Modificar con paleta Punto (-) para disminuir / Raya (+) para aumentar.
2. Modo Iambico: Alternar entre IAM-A y IAM-B.
   * Seleccionar con Punto (Modo A) / Raya (Modo B).

* Navegacion: Una pulsacion corta en el boton de comando conmuta entre las opciones del menu. Al pasar de la ultima opcion, el sistema guarda automaticamente los parametros en la EEPROM, sale del menu y muestra la pantalla principal con la leyenda `>SAVED<`.

---

## Parametros por Defecto

* Velocidad: 20 WPM
* Modo Iambico: Modo B (`iambicModeA = false`)
* Sidetone: Habilitado (600 Hz en operacion, 1000Hz/1200Hz para alertas del sistema)
