# Keyer Iambico, Decodificador CW y Modo Trainer - ESP32-C3 Mini by LU2FTI

Sistema completo de manipulacion telegrafica iambico (Modos A y B), decodificador CW en tiempo real y **Modo Trainer interactivo** con pantalla OLED de 1.3 pulgadas (Controlador SH1106), disenado para el microcontrolador ESP32-C3 Mini.

---

## Caracteristicas Principales

* Manipulador Iambico Doble: Soporte seleccionable para Modo A y Modo B.
* Decodificador CW en Tiempo Real: Analiza y muestra los caracteres decodificados directamente en pantalla a partir de la manipulacion.
* Modo Trainer Integrado: Interfaz de practica dedicada con visualizacion de letra objetivo gigante y letra tipeada lado a lado, evaluacion en tiempo real, tono de acierto ascendente y reintentos libres.
* Pantalla OLED 1.3" (SH1106): Interfaz visual basada en la libreria U8g2, con visualizacion nitida de texto y barras de estado.
* Menu Interactivo Integrado: Permite ajustar la velocidad (WPM), el modo iambico y el estado del trainer sin reprogramar el chip.
* Memoria No Volatil (EEPROM): Guarda automaticamente toda la configuracion en la EEPROM interna del ESP32-C3.
* Sidetone y Salida TX: Salida de tono local (sidetone) configurable y linea de salida para activacion de transmisor (Key Out).

---

## Diagrama de Conexiones y Pinout (ESP32-C3 SuperMini)

Disenado fisicamente tomando como referencia la disposicion de pines bilateral de la placa ESP32-C3 SuperMini:

```text
                           [ USB-C ]
                       +---------------+
          (5V / VBUS)  | 5V        IO4 | ---> OLED SCL (I2C_SCL)
          (GND Comun)  | GND       IO5 | ---> OLED SDA (I2C_SDA)
          (Alim. 3.3V) | 3V3       IO6 | ---> Salida TX / Key Out
      (Audio Sidetone) | GPIO1     IO7 | [ ]
         (Boton Menu)  | GPIO0     IO8 | [ ]
  (Paleta DIT / Punto) | GPIO2     IO9 | [ ]
   (Paleta DAH / Raya) | GPIO3     GND | ---> GND Comun
                       +---------------+

```

| Pin en Codigo | GPIO ESP32-C3 | Funcion / Periferico |
| --- | --- | --- |
| `PIN_DIT` | GPIO 2 | Paleta de Punto (Dit) (Pull-up interno) |
| `PIN_DAH` | GPIO 3 | Paleta de Raya (Dah) (Pull-up interno) |
| `PIN_COMMAND` | GPIO 0 | Boton de Comando / Menu (Pull-up interno) |
| `PIN_SIDETONE` | GPIO 1 | Salida de Tono Sidetone (Buzzer / Audio) |
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



### 2. Modo Trainer (Entrenamiento)

Cuando esta activo, la pantalla muestra en grande la letra objetivo a la izquierda y la letra decodificada por tus paletas a la derecha.

* Evaluacion en Tiempo Real: Si escribes la letra correcta, el sistema emite un acorde ascendente de acierto y cambia automaticamente de letra.
* Reintentos: Si le erras, se emite un tono grave de error y puedes seguir intentando con la misma letra hasta pegarle.
* Boton de Comando: Una pulsacion corta mientras estas en el modo trainer te permite saltar manualmente a otra letra de practica.

### 3. Menu Interactivo

Al ingresar al menu (pulsacion larga del boton), la pantalla mostrara las 3 opciones disponibles:

1. Velocidad (WPM): Ajustable de 5 a 50 WPM (modificar con paleta Punto `-` para disminuir / Raya `+` para aumentar).
2. Modo Iambico: Alternar entre `IAM-A` y `IAM-B` (Punto / Raya).
3. Trainer: Activar (`ON`) o Desactivar (`OFF`) el modo entrenamiento.

* Navegacion: Una pulsacion corta en el boton de comando conmuta entre las opciones del menu. Al pasar de la ultima opcion, el sistema guarda automaticamente los parametros en la EEPROM, sale del menu y muestra la pantalla principal (o el trainer si quedo activado) con la leyenda `>SAVED<`.

---

## Parametros por Defecto

* Velocidad: 20 WPM
* Modo Iambico: Modo B (`iambicModeA = false`)
* Modo Trainer: Desactivado (`trainerEnabled = false`)
* Sidetone: Habilitado (600 Hz en operacion, tonos dinamicos para alertas y aciertos en trainer)
