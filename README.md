# PsocDuino

Núcleo Arduino para microcontroladores **Cypress PSoC 1** (familia M8C): CY8C21234,
CY8C21534, CY8C24223A, CY8C24423A, CY8C27243, CY8C27443 y CY8C29466 listos para usar,
y 55 modelos en total para el programador. Compila con el **ImageCraft** de PSoC
Designer y graba con un programador **CH341A** por ISSP, todo desde el **Arduino IDE 2.x**.

Proyecto de [MikrocontrollerCephs](https://www.youtube.com/@mikrocontrollersceph)
(GitHub: [CEPH81](https://github.com/CEPH81)).

---

## Contenido

1. [Estado del proyecto](#1-estado-del-proyecto)
2. [Qué se necesita](#2-qué-se-necesita)
3. [Instalación](#3-instalación)
4. [Conexión del CH341A al PSoC](#4-conexión-del-ch341a-al-psoc)
5. [Placas y menús del IDE](#5-placas-y-menús-del-ide)
6. [Compilar y subir](#6-compilar-y-subir)
7. [Patas (pines)](#7-patas-pines)
8. [Funciones disponibles (API)](#8-funciones-disponibles-api)
9. [Periféricos: Serial, analogRead, analogWrite, dacWrite, I2C](#9-periféricos-serial-analogread-analogwrite-dacwrite-i2c)
10. [SPI](#10-spi)
11. [Frecuencia de la CPU y tensión](#11-frecuencia-de-la-cpu-y-tensión)
12. [Tiempos: millis, delay, delayMicroseconds](#12-tiempos-millis-delay-delaymicroseconds)
13. [Memoria y cómo ahorrarla](#13-memoria-y-cómo-ahorrarla)
14. [Diferencias con el Arduino normal (lenguaje C89)](#14-diferencias-con-el-arduino-normal-lenguaje-c89)
15. [Ejemplos incluidos](#15-ejemplos-incluidos)
16. [Configuración de PSoC Designer de las placas con periféricos](#16-configuración-de-psoc-designer-de-las-placas-con-periféricos)
17. [Agregar otro chip (importar una variante)](#17-agregar-otro-chip-importar-una-variante)
18. [psocprog (programador por línea de órdenes)](#18-psocprog-programador-por-línea-de-órdenes)
19. [Solución de problemas](#19-solución-de-problemas)
20. [Cómo funciona por dentro](#20-cómo-funciona-por-dentro)
21. [Desarrollo y publicación](#21-desarrollo-y-publicación)

---

## 1. Estado del proyecto

| Parte | Estado |
|---|---|
| Programador CH341A `psocprog` (Windows / Linux / macOS) | ✅ listo; conexión con el CH341A probada |
| Instalación por el Gestor de placas | ✅ (versión actual en `platform.txt`) |
| Compilación con ImageCraft (`psocbuild`) | ✅ los 7 chips compilan con el ImageCraft real |
| Pines, `millis`, `delay`, `shiftOut`, `pulseIn`, matemáticas… | ✅ |
| `Serial`, `analogRead`, `analogWrite` (PWM), `dacWrite` | ✅ placas “+ perifericos” (pendiente de prueba en el chip) |
| I2C por hardware | ✅ módulo I2CHW en CY8C27443 / CY8C29466 + periféricos (sin biblioteca `Wire` todavía) |
| I2C por software | ✅ ejemplos (cualquier pata) |
| SPI | ✅ por software (`shiftOut`/`shiftIn`); 🔜 por hardware (SPIM) en 27443/29466 |
| Subida de un programa al chip real | 🔜 en pruebas |
| `attachInterrupt`, más canales PWM, biblioteca `Wire`/`SPI` | 🔜 |

Hoja de ruta: [docs/ROADMAP.md](docs/ROADMAP.md).

---

## 2. Qué se necesita

| Elemento | Para qué | Notas |
|---|---|---|
| **Arduino IDE 2.x** | editar, compilar y subir | también funciona con `arduino-cli` |
| **PSoC Designer 5.4** (gratuito, de Cypress/Infineon) | trae el compilador **ImageCraft ICCM8C** | **solo Windows**. Compilar requiere Windows; subir un `.hex` funciona en Windows, Linux y macOS |
| Programador **CH341A** (“mini”, el de las memorias 25xx) | grabar el chip por ISSP | jumper en **1–2** (modo programador) |
| Driver del CH341A (Windows) | que Windows vea el programador | **CH341PAR** de WCH, o el `CH341WDM.INF` del programa original |
| Un PSoC 1 | — | cables de menos de 15 cm |

---

## 3. Instalación

### 3.1 Gestor de placas (recomendado)

1. **Archivo → Preferencias → URLs adicionales de gestor de placas**, agregue:
   ```
   https://raw.githubusercontent.com/CEPH81/PsocDuino/main/package_psocduino_index.json
   ```
2. **Herramientas → Placa → Gestor de placas**, busque **PsocDuino** e instale la
   última versión. Para actualizar, vuelva al Gestor y pulse **Actualizar**.
3. Se instala en `%LOCALAPPDATA%\Arduino15\packages\psocduino\hardware\m8c\<versión>\`
   con todo incluido: núcleo, variantes de los 7 chips, bibliotecas, ejemplos,
   `psocbuild.exe`, `psocprog.exe` y `CH341DLL.DLL`.

### 3.2 Driver del CH341A (una sola vez)

- **Windows:** instale **CH341PAR** de WCH, o clic derecho en `CH341WDM.INF`
  (carpeta `CH341A driver\CH341_EPP_MEM_I2C_IO\` del programa original) → Instalar.
  Si `psocprog` no encuentra la DLL, prueba con libusb (driver WinUSB instalado con
  **Zadig**) y, si falla, muestra un diagnóstico en español (jumper, driver, Zadig).
- **Linux:** copie `tools/udev/99-ch341a-psocduino.rules` a `/etc/udev/rules.d/`
  y ejecute `sudo udevadm control --reload && sudo udevadm trigger`.
- **macOS:** usa libusb, no necesita driver.

### 3.3 PSoC Designer 5.4

Instálelo en la carpeta por defecto. `psocbuild` lo encuentra solo. Si está en otra
carpeta, defina la variable de entorno **`PSOC_DESIGNER_DIR`** con la ruta de
`...\PSoC Designer\5.4\Common\CypressSemiBuildMgr` y reinicie el IDE.

### 3.4 Instalación manual (para desarrollo)

Clone el repositorio en `Documentos\Arduino\hardware\psocduino\m8c\`. Los programas
compilados ya vienen en `tools\bin\<sistema>\`.

---

## 4. Conexión del CH341A al PSoC

Se usa el zócalo de 8 pines (25xx) del CH341A:

| CH341A (zócalo 25xx) | Señal CH341 | PSoC 1 | Notas |
|---|---|---|---|
| 1 CS | D0 | **XRES** (modo XRES) o control de Vdd (modo POR) | |
| 6 CLK | D3 | **SCLK** = P1[1] | |
| 5 MOSI | D5 | **SDATA** = P1[0] a través de **1 kΩ** | el programador escribe |
| 2 MISO | D7 | **SDATA** = P1[0] directo | el programador lee |
| 4 GND | GND | Vss | |
| 8 VCC | 5 V / 3,3 V | Vdd | según el jumper de tensión del CH341A |

```
 CH341A                       PSoC 1
 MOSI (D5) ──[ 1 kΩ ]──┬───── SDATA  P1[0]
 MISO (D7) ────────────┘
 CLK  (D3) ────────────────── SCLK   P1[1]
 CS   (D0) ────────────────── XRES   (o base del transistor de Vdd en modo POR)
 GND       ────────────────── Vss
 VCC       ────────────────── Vdd
```

- La tensión de las patas del CH341A debe coincidir con la Vdd del PSoC y con el menú
  *Alimentación del chip*.
- **Modo POR / encendido** (chips sin XRES, o si XRES no es accesible): D0 controla la
  Vdd del PSoC con un transistor PNP (D0 = 0 → alimentado). `psocprog` apaga la Vdd
  100 ms, la enciende y entra en programación en los primeros microsegundos.
- **Circuito con selector** (diagrama 4 de antiradio.narod.ru): transistor **Q1** que
  conmuta la Vdd, interruptor **S1** para el modo POR y **SA1** para elegir la fuente.

  | S1 | Menú *Entrada a programación* |
  |---|---|
  | POR activado | **Encendido / POR** |
  | POR desactivado | **XRES** |

  En modo POR deje el menú en **5 V**: por la caída en Q1 el chip queda a unos 4,5 V.
- Mantenga los cables cortos (< 15 cm). P1[0] y P1[1] no deben tener cargas fuertes
  en la placa (LEDs, condensadores) o la programación falla.

Más detalles: [docs/conexion.md](docs/conexion.md).

---

## 5. Placas y menús del IDE

### 5.1 Placas (Herramientas → Placa → PsocDuino)

**Placas sin periféricos** (solo pines y tiempos; usan menos memoria):

| Placa | Flash | RAM | Encapsulado | Puertos | Memoria |
|---|---|---|---|---|---|
| PSoC 1 CY8C21234 | 8 KB | 512 B | 16 pines | P0, P1 (parcial) | grande (LMM, 2 páginas) |
| PSoC 1 CY8C21534 | 8 KB | 512 B | 28 pines | P0–P3 | grande (LMM, 2 páginas) |
| PSoC 1 CY8C24223A | 4 KB | 256 B | 20 pines | P0, P1 (**sin P2**) | pequeña (SMM) |
| PSoC 1 CY8C24423A | 4 KB | 256 B | 28 pines | P0–P2 | pequeña |
| PSoC 1 CY8C27243 | 16 KB | 256 B | 20 pines | P0, P1 (parcial) | pequeña |
| PSoC 1 CY8C27443 | 16 KB | 256 B | 28 pines | P0–P2 | pequeña |
| PSoC 1 CY8C29466 | 32 KB | 2 KB | 28 pines | P0–P2 | grande (LMM, 8 páginas) |

**PSoC 1 (todos los modelos):** una sola placa con el menú **Herramientas →
Microcontrolador** para cambiar entre los 7 chips de arriba sin cambiar de placa.

**Placas con periféricos** (usan los User Modules de los proyectos Base2 de PSoC
Designer; necesarias para la biblioteca **PsocDuinoPerif**):

| Placa | UART (Serial) | PWM | ADC | DAC | I2C hardware |
|---|---|---|---|---|---|
| PSoC 1 CY8C24423A + perifericos | TX P2_1, RX P2_0 | P1_6 | P0_1, P0_3, P0_7 | P0_5 (DAC6, 64 escalones) | — |
| PSoC 1 CY8C24223A + perifericos | TX P1_4, RX P1_2 | P1_6 | P0_1, P0_3, P0_7 | P0_5 (DAC8, 256 escalones) | — |
| PSoC 1 CY8C27443 + perifericos | TX P2_1, RX P2_0 | P1_6 | P0_1, P0_3, P0_7 | P0_5 (DAC8) | SDA P1_5, SCL P1_7 |
| PSoC 1 CY8C29466 + perifericos | TX P2_1, RX P2_0 | P1_6 | P0_1, P0_3, P0_7 | P0_5 (DAC8) | SDA P1_5, SCL P1_7 |

> Las mismas cuatro también están en el menú **Microcontrolador** de “PSoC 1 (todos los
> modelos)” como **“CY8C24423A + perifericos”**, etc. (desde la v0.5.3).

### 5.2 Menús de Herramientas

| Menú | Opciones | Qué hace |
|---|---|---|
| **Microcontrolador** (solo “todos los modelos”) | los 7 chips y los 4 “+ perifericos” | elige variante, tamaño de memoria y chip para `psocprog` |
| **Alimentación del chip** | **5 V** / 3,3 V | calibración del oscilador (IMO) y del bandgap para esa tensión, nivel del detector de baja tensión (LVD) y tensión de programación |
| **Reloj de CPU** | **24** / 12 / 6 / 3 / 1,5 MHz | divisor de la CPU (SysClk/1, /2, /4, /8, /16). Ver [sección 11](#11-frecuencia-de-la-cpu-y-tensión) |
| **Entrada a programación** | **Encendido / POR** / XRES | cómo `psocprog` pone el chip en modo programación |
| **Seguridad de flash** | **Sin protección** / Según el .hex / Proteger todo | bits de protección escritos después de verificar. *Sin protección* permite volver a leer la flash |
| **Verificación** | **Completa (byte a byte)** / Rápida (checksum) | comprobación después de grabar |
| **Programador** | **CH341A (psocprog, ISSP)** | el único disponible |

En negrita, la opción por defecto.

---

## 6. Compilar y subir

1. Elija la placa y los menús (sección 5).
2. **Herramientas → Programador → CH341A (psocprog, ISSP)**.
3. **Verificar** (✓) para compilar, o **Subir** (→) para compilar y grabar. No hace falta
   elegir puerto.
4. **Herramientas → Quemar bootloader** borra el chip y comprueba que responde.

En la salida aparece el chip con el que se trabaja:

```
PsocDuino: microcontrolador CY8C24423A  (variante cy8c24423a, 5 V, CPU 24 MHz)
...
Microcontrolador seleccionado: CY8C24423A (4K flash) entrada=power Vdd=5
```

Al subir, `psocprog` lee el ID de silicio del chip y **avisa si no coincide** con la
placa elegida. Para ver las órdenes completas active en **Preferencias** *Mostrar
salida detallada durante: compilación / carga*.

---

## 7. Patas (pines)

### 7.1 Nombres

Las patas se nombran **`Px_y`** (puerto x, bit y): `P0_0` … `P7_7`. El número interno
es `puerto × 8 + bit` (`P1_2` = 10). `LED_BUILTIN` = `P1_2`.

Solo existen las patas del encapsulado de su chip (vea la hoja de datos). Escribir en
una pata de un puerto que el chip no tiene se ignora.

### 7.2 Uso digital: cualquier pata, como en Arduino

`pinMode`, `digitalWrite`, `digitalRead`, `digitalToggle`, `shiftOut`, `shiftIn` y
`pulseIn` funcionan **en cualquier pata**.

| Modo de `pinMode` | Modo de la pata del PSoC |
|---|---|
| `OUTPUT` | salida fuerte (push-pull) |
| `INPUT` | entrada digital en alta impedancia |
| `INPUT_PULLUP` | resistencia de 5,6 kΩ a Vdd |
| `INPUT_PULLDOWN` | resistencia de 5,6 kΩ a GND |
| `OUTPUT_OPEN_DRAIN` | drenador abierto: LOW = a GND, HIGH = suelta (para I2C) |
| `INPUT_ANALOG` | alta impedancia analógica, sin buffer digital |

> En el PSoC, con `INPUT_PULLUP`/`INPUT_PULLDOWN` el latch de datos elige la
> resistencia: `pinMode` lo hace solo. Cambiar con `digitalWrite` una pata en
> pull-up cambia el tipo de resistencia, igual que en el Arduino AVR.

### 7.3 Patas que hay que cuidar

| Patas | Por qué |
|---|---|
| **P1_0, P1_1** | son SDATA y SCLK del programador. Se pueden usar en el programa, pero lo que conecte ahí puede impedir grabar |
| **Placas “+ perifericos”**: P2_0/P2_1 (UART; en el 24223A P1_2/P1_4), P1_6 (PWM), P0_5 (DAC), P1_5/P1_7 (I2C en 27443/29466) | las usa un periférico. Si las toca con `pinMode`/`digitalWrite`, el periférico se desconecta de la pata |
| P0_1, P0_3, P0_7 en las placas con periféricos | entradas del ADC: `analogRead` las pone solo en `INPUT_ANALOG` |

### 7.4 Funciones analógicas: solo en patas fijas

En el PSoC 1 cada periférico va **cableado a ciertas patas** dentro del proyecto de
PSoC Designer. No es como en Arduino, donde cualquier pata A0–A5 sirve.

| Función | Patas | Si se usa otra pata |
|---|---|---|
| `analogRead()` | **P0_1, P0_3, P0_7** (P0_5 también, pero es la salida del DAC) | devuelve **0**. El multiplexor de la columna analógica 0 solo llega a P0_1/P0_3/P0_5/P0_7 |
| `analogWrite()` | **P1_6** (PWM por hardware, 31,25 kHz) | se comporta como `digitalWrite`: HIGH si el valor es ≥ 128, LOW si es menor |
| `dacWrite()` | **P0_5** | — |
| `Serial` | ver tabla de la sección 5.1 | — |

Para usar otras patas hay que cambiar el enrutado en PSoC Designer y volver a importar
la variante (sección 17). Ejemplos de lo que se puede: el PWM (salida 2 de la fila 0)
también puede salir por P1_2, P0_2, P0_6, P2_2 o P2_6; en el 27443 y el 29466 hay filas
digitales libres para más PWM, timers o SPI.

---

## 8. Funciones disponibles (API)

Se incluye `Arduino.h` automáticamente.

### 8.1 Pines
`pinMode(pin, modo)`, `digitalWrite(pin, HIGH/LOW)`, `digitalRead(pin)`,
`digitalToggle(pin)` (invierte la salida, extra de PsocDuino).

### 8.2 Tiempo
`millis()`, `micros()`, `delay(ms)`, `delayMicroseconds(us)` (sección 12).

### 8.3 Comunicación por software
`shiftOut(datos, reloj, MSBFIRST/LSBFIRST, valor)`,
`shiftIn(datos, reloj, orden)`, `pulseIn(pin, estado, timeout_us)`.

### 8.4 Matemáticas
`map()`, `constrain()`, `min()`, `max()`, `abs()`, `sq()`, `randomSeed()`,
`random(max)`, **`randomRange(min, max)`** (en lugar de `random(min, max)`, porque C
no tiene sobrecarga).

### 8.5 Bits y bytes
`bit()`, `bitRead()`, `bitSet()`, `bitClear()`, `bitWrite()`, `lowByte()`,
`highByte()`.

### 8.6 Interrupciones
`interrupts()`, `noInterrupts()`. (`attachInterrupt` todavía no.)

### 8.7 Tipos
`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`, `byte`, `word`,
`boolean`, `bool`. En ImageCraft **`int` es de 16 bits** y **`long` de 32 bits**.

### 8.8 Macros útiles
`F_CPU` (frecuencia de la CPU elegida), `PSOC_NUM_PORTS`, `NUM_DIGITAL_PINS`,
`LED_BUILTIN`, `clockCyclesPerMicrosecond()`, y `ARDUINO_PSOC_CY8C24423A` (u otro
chip) para escribir código distinto por chip:

```c
#ifdef ARDUINO_PSOC_CY8C29466
  /* solo en el 29466 */
#endif
```

---

## 9. Periféricos: Serial, analogRead, analogWrite, dacWrite, I2C

Solo en las placas **“+ perifericos”**. En el sketch:

```c
#include <PsocDuinoPerif.h>
```

Hay que elegir una placa **“+ perifericos”** o, en “PSoC 1 (todos los modelos)”, un
**Microcontrolador “... + perifericos”**. Si no, el IDE muestra la variante en uso y:
`PsocDuino: la placa elegida no tiene perifericos ...`.

### 9.1 Serial (UART 8N1)

```c
Serial.begin(115200);        /* 115200, 57600, 38400, 19200, 9600 o 4800 */
Serial.print("texto");       /* solo textos entre comillas (van en la flash) */
Serial.println("texto");
Serial.printNum(n);          /* numeros (long) */
Serial.printlnNum(n);
Serial.write(c);             /* un byte */
if (Serial.available()) c = Serial.read();   /* read() da -1 si no hay dato */
Serial.flush();              /* espera a que salga el ultimo byte */
Serial.end();
```

- `Serial` es una estructura de C (ImageCraft no compila C++); por eso los números
  van con `printNum`/`printlnNum` y no con `print`.
- Velocidades: 115200 (por defecto), 57600, 38400 y 19200 salen de SysClk; 9600 y 4800
  salen de VC2. Cualquier otro valor mayor que 4800 queda en 115200.
- **Recepción de un solo byte, sin interrupción:** `available()` guarda un byte. A
  115200 lea con frecuencia, o use una velocidad menor si el PC envía muchos datos
  seguidos.
- Conexión con un adaptador USB-serie de la misma tensión que el PSoC: TX del PSoC →
  RX del adaptador, RX → TX, y las tierras unidas.

### 9.2 analogRead

```c
int v = analogRead(P0_3);    /* 0..1023 (10 bits por defecto) */
analogReadResolution(12);    /* 6 a 12 bits: 12 -> 0..4095 */
```

- ADC incremental **ADCINC de 12 bits** con un **PGA de ganancia 1**. Rango de
  **0 V a Vdd**: referencia analógica Vdd/2 ± Vdd/2 (desde la v0.6.1; antes era
  Vdd/2 ± bandgap, de 1,2 a 3,8 V con 5 V). A 5 V cada paso de 12 bits es de 1,22 mV.
- Para pasar a milivoltios sin coma flotante: `mv = ((long)v * VDD_MV) >> 12` con
  `analogReadResolution(12)`; `PSOC_VCC` vale 5 o 3 según el menú *Alimentación*.
- Entradas: P0_1, P0_3, P0_7 (y P0_5). Otras patas devuelven 0.
- Si el ADC no responde, devuelve 0 después de un tiempo en lugar de colgarse.

### 9.3 analogWrite (PWM)

```c
analogWrite(P1_6, 128);      /* 0..255, 31,25 kHz, por hardware */
```

Una sola salida PWM real, en **P1_6**. En otras patas `analogWrite` funciona como
salida digital (≥ 128 = HIGH).

### 9.4 dacWrite

```c
dacWrite(200);               /* 0..255 -> 0 V..Vdd en P0_5 */
```

En el CY8C24423A el DAC es de 6 bits: el valor 0..255 se convierte a 64 escalones. En
los demás es de 8 bits (256 escalones).

### 9.5 I2C por hardware (CY8C27443 y CY8C29466 + periféricos)

Módulo **I2CHW** maestro en **SDA P1_5, SCL P1_7**. Necesita resistencias de
**4,7 kΩ** de SDA y SCL a Vdd. Todavía no hay biblioteca `Wire`: use las funciones
de PSoC Designer:

```c
#include <PsocDuinoPerif.h>
#include "I2CHW_1Mstr.h"
/* I2CHW_1_Start(); I2CHW_1_EnableMstr(); I2CHW_1_bWriteBytes(dir, buf, n, I2CHW_1_CompleteXfer); ... */
```

En **todos los chips** se puede usar el **I2C por software** de los ejemplos
`I2C_EEPROM_24C256` e `I2C_Teclado_MPR121` (pestaña `I2C_soft.ino`, ~50 kHz, en
cualquier par de patas).

### 9.6 Memoria de la biblioteca

Las rutinas de los User Modules se enlazan desde una biblioteca: **solo ocupan flash
las que usa el sketch**. Las rutinas de interrupción y la tabla de configuración entran
siempre.

---

## 10. SPI

### 10.1 Por software (todas las placas, ya disponible)

Use `shiftOut()`/`shiftIn()` en **cualquier pata** (modo 0, MSB o LSB primero), como en
el ejemplo **Registro_74HC595**. Es más lento (cientos de kHz a 24 MHz) y la CPU queda
ocupada mientras transmite, pero no gasta bloques digitales.

```c
digitalWrite(CS, LOW);
shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, dato);
digitalWrite(CS, HIGH);
```

### 10.2 Por hardware (User Module SPIM, en preparación)

El SPIM ocupa un bloque de comunicaciones (DCB). En el CY8C24423A/24223A los 4 bloques
digitales ya están ocupados (ADC, PWM, UART), así que ahí se usa SPI por software. En
el **CY8C27443** y el **CY8C29466** hay filas libres. Configuración prevista (fila 1):

| Señal | Pata | Bus |
|---|---|---|
| SS | P2_2 | StdCPU (lo maneja el sketch) |
| SCK | P2_3 | Row_1_Output_3 → GlobalOutEven_3 |
| MISO | P2_4 | GlobalInEven_4 → Row_1_Input_0 |
| MOSI | P2_5 | Row_1_Output_1 → GlobalOutEven_5 |

SPIM en **DCB12**, reloj **VC2** (SCK = 4 MHz), *Interrupt Mode* TXRegEmpty,
*ClockSync* Sync to SysClk, *InvertMISO* Normal. Patas: P2_3 y P2_5 *Strong*, P2_4
*High Z*, P2_2 *StdCPU Strong*. Registros esperados: `RDI1SRO0 = 20h`,
`RDI1SRO1 = 10h`, `PRT2GS = 3Bh`. El modo SPI (0–3) y el orden de bits los elige
`SPIM_Start()` desde el programa (futuro `SPI.setDataMode()`/`SPI.setBitOrder()`).

---

## 11. Frecuencia de la CPU y tensión

El menú **Reloj de CPU** (24, 12, 6, 3 o 1,5 MHz) solo cambia el divisor **de la CPU**
(`CPU_CLOCK`). El reloj del sistema **SysClk sigue en 24 MHz** (oscilador interno IMO),
y de él salen VC1, VC2 y VC3.

**No cambia nada en:**
- las velocidades de `Serial` (VC3 / VC2);
- la frecuencia del PWM (31,25 kHz) y el reloj del ADC (VC2 = 8 MHz);
- el DAC;
- `millis()` (Sleep Timer a 512 Hz, independiente de la CPU) y `delay()` (se recalcula al
  compilar con la frecuencia elegida);
- la grabación con el CH341A.

**Sí cambia:**
- **La velocidad del programa**: a 12 MHz va a la mitad; a 1,5 MHz, dieciséis veces más
  lento. El consumo baja en la misma proporción.
- **`delayMicroseconds()`** se ajusta solo (usa `F_CPU` al compilar), pero a 3 MHz o
  1,5 MHz los retardos menores de unos 50–100 µs salen más largos de lo pedido.
- **`Serial` rápido**: como recibe un byte por vez sin interrupción, a 6 MHz o menos el
  programa puede no alcanzar a leer cada byte a 115200. Use 9600 o 19200.
- I2C por software, `shiftOut` y los bucles de `digitalWrite` generan señales más lentas
  (siguen funcionando).

**Tensión:** a **3,3 V** la hoja de datos admite como máximo **12 MHz** de CPU. Si elige
3,3 V con 24 MHz el IDE compila pero muestra un aviso. El menú *Alimentación* ajusta
`POWER_SETTING` (5 V: 10h, 3,3 V: 08h), el LVD y la tensión de programación.

**Lo que no funcionaría igual:** cambiar el **SysClk** mismo en PSoC Designer (cristal
externo, SysClk×2 u otro modo del oscilador). Eso cambiaría las velocidades del UART, el
PWM y el ADC, y habría que ajustar las tablas de la biblioteca.

---

## 12. Tiempos: millis, delay, delayMicroseconds

- **`millis()` / `micros()`**: Sleep Timer a **512 Hz** desde el oscilador ILO
  (15–64 kHz, impreciso de fábrica). Al arrancar se mide un tic contra el reloj de la
  CPU y se corrige: queda preciso aunque el ILO no lo sea. **Resolución de un tic
  (~2 ms)** tanto en `millis()` como en `micros()`: para medir microsegundos use
  `pulseIn()` o un contador propio.
- **`delay(ms)`**: repite `delayMicroseconds(1000)`; tiene la precisión del IMO (±2,5 %)
  y no depende del ILO.
- **`delayMicroseconds(us)`**: bucles de ciclos exactos (bloques de 8 µs y de 1 µs)
  sobre el IMO (±2,5 %). Se ajusta con dos constantes:
  - `PSOCDUINO_LOOP_CYCLES` (por defecto 30): ciclos de cada bloque; más alto = pulso más corto;
  - `PSOCDUINO_CALL_CYCLES` (por defecto 60): costo de la llamada.

  Mídalo con el ejemplo **CalibrarDelay** y un osciloscopio (P1_2 debe medir 1,000 ms;
  P1_3 tiene un período de 200 ms). Para cambiar una constante cree
  `platform.local.txt` junto a `platform.txt` con:
  ```
  compiler.c.extra_flags=-DPSOCDUINO_LOOP_CYCLES=NN
  ```

---

## 13. Memoria y cómo ahorrarla

El CY8C24423A tiene solo **4 KB de flash y 256 B de RAM**, así que PsocDuino está hecho
para ocupar lo mínimo:

- El núcleo está dividido en archivos pequeños (`wiring_digital.c`, `wiring_read.c`,
  `wiring_toggle.c`, `wiring_shift.c`, `wiring_pulse.c`, `wiring_math.c`,
  `wiring_millis.c`, `wiring_micros.c`…) dentro de una biblioteca: **solo se enlaza lo
  que el sketch usa**.
- Los accesos a registros se hacen con rutinas en ensamblador compartidas.
- **Placas sin periféricos:** la configuración de PSoC Designer solo repetía los valores
  de reset de los registros, así que no se enlaza (`LoadConfigInit` vacío): ~250 bytes
  menos. Si importa una variante con patas configuradas a mano en PSoC Designer, se usa
  la configuración completa automáticamente (o con `FULLCONFIG=1` en `variant.cfg`).
- La calibración de `millis()` usa solo restas y desplazamientos (sin las rutinas de
  multiplicar y dividir de 16 bits), y `pinMode` usa una tabla de modos.
- Los User Modules también se enlazan desde biblioteca (`libmodulos.a`).
- Al compilar se muestra el uso real de flash y RAM. Si no cabe, el mensaje dice cuántos
  bytes faltan y deja el mapa `.mp` en la carpeta de compilación.

**Consejos para su sketch:**
- Tablas y textos fijos con **`const`** → van a la flash, no a la RAM.
- Prefiera `unsigned char`/`int` en lugar de `long` (las operaciones de 32 bits
  agregan rutinas de la biblioteca matemática).
- Evite `/` y `%` con variables; con constantes potencia de 2 use `>>` y `&`.
- No use `float` en los chips de 4 KB.
- `Serial.print` con textos entre comillas (van en flash); los números, con `printNum`.

---

## 14. Diferencias con el Arduino normal (lenguaje C89)

ImageCraft solo compila **C89**. `psocbuild` convierte el `.ino` en `.c`, pero:

| En Arduino | En PsocDuino |
|---|---|
| Clases y objetos (`String`, `Wire.begin()`, `lcd.print()`) | no hay clases. `Serial` es una estructura con punteros a función |
| Sobrecarga: `random(a, b)`, `Serial.print(123)` | `randomRange(a, b)`, `Serial.printNum(123)` |
| Declarar variables en cualquier parte, `for (int i = 0; ...)` | declárelas **al inicio de cada bloque**: `int i; for (i = 0; ...)` |
| Comentarios `//` | funcionan (ImageCraft los acepta) |
| `const char *` puede apuntar a RAM | en ImageCraft **`const` = flash**: no pase un arreglo de RAM a un parámetro `const char *` |
| `int` de 16 bits (AVR) | igual: `int` 16 bits, `long` 32 bits |
| `setup()` / `loop()` | igual (`psocbuild` les agrega `void` para evitar avisos MISRA) |
| Varias pestañas `.ino` | funcionan (se unen en un solo archivo) |

---

## 15. Ejemplos incluidos

**Archivo → Ejemplos → PsocDuino** (todas las placas):

| Ejemplo | Qué hace | Patas |
|---|---|---|
| Blink | parpadea un LED | P1_2 |
| Boton | pulsador a GND enciende un LED | botón P0_7 (pull-up), LED P1_2 |
| CalibrarDelay | calibrar `delayMicroseconds` con osciloscopio | P1_2, P1_3 |
| Registro_74HC595 | contador en 8 LEDs con `shiftOut` (SPI por software) | DS P1_3, SH_CP P1_4, ST_CP P1_2 |
| LCD_HD44780 | pantalla 16×2 en modo 4 bits | D4–D7 P2_0–P2_3, E P2_4, RS P2_5, R/W P2_6 |
| I2C_EEPROM_24C256 | cuenta arranques en una EEPROM 24C256 (0x50), I2C por software | SDA P1_5, SCL P1_7, LED P1_6 |
| I2C_Teclado_MPR121 | teclado táctil de 9 teclas MPR121 (0x5A), I2C por software | SDA P1_5, SCL P1_7, LED P1_6 |

**Archivo → Ejemplos → PsocDuinoPerif** (solo placas “+ perifericos”):

| Ejemplo | Qué hace | Patas |
|---|---|---|
| Serial_Eco | eco por el UART a 115200 | ver tabla 5.1 |
| Potenciometro_ADC | lee un potenciómetro y envía el valor (12 bits), los mV y el % de giro por Serial | cursor en P0_3 |
| PWM_Fade | un LED sube y baja de brillo | LED con 330 Ω en P1_6 |
| DAC_Rampa | rampa de 0 V a Vdd (osciloscopio); un potenciómetro cambia la velocidad | salida P0_5, potenciómetro P0_3 |
| LM35_Temperatura | temperatura con LM35 (10 mV/°C), un decimal, promedio de 16 lecturas; necesita Vdd = 5 V | Vout del LM35 en P0_1 |
| NTC_Temperatura | temperatura con NTC 10 k (B 3950) y resistencia fija de 10 k; tabla de -20 a 105 °C con interpolación (error < 0,3 °C), avisa si el sensor está abierto o en corto | divisor en P0_7 |

Los tres de sensores (Potenciometro_ADC, LM35_Temperatura y NTC_Temperatura) usan
patas distintas (P0_3, P0_1 y P0_7), así que se pueden conectar a la vez. Ninguno usa
coma flotante: caben en los chips de 4 KB.

Los ejemplos con patas del puerto 2 son para los chips de 28 pines. En los de 16/20
pines (CY8C21234, CY8C24223A, CY8C27243) cambie las patas por otras que existan.

---

## 16. Configuración de PSoC Designer de las placas con periféricos

Proyectos `Base2_<chip>` (lenguaje C, **Generate + Build sin errores**).

### 16.1 Global Resources (todos)

| Parámetro | Valor |
|---|---|
| CPU_Clock | SysClk/1 (el menú del IDE lo cambia) |
| Sleep_Timer | **512_Hz** (lo usa `millis()`) |
| Power Setting | 5.0V / 24MHz (el menú del IDE lo cambia) |
| VC1 | SysClk/1 |
| VC2 | VC1/3 → **8 MHz** |
| VC3 | Source SysClk, divider **26** → 115 200 baudios |
| AnalogColumn_Clock_0..3 | **VC2** |

### 16.2 User Modules

| Módulo | 24423A | 24223A | 27443 | 29466 |
|---|---|---|---|---|
| ADCINC (12 bits, Unsigned, Data Clock VC2) | contador DBB00 + ASC10 + ASD20 | igual | igual | contador **DBB01** + ASC10 + ASD20 |
| PGA (ACB00, **Gain 1.000**, AnalogColumn_InputMUX_0, AnalogBus Disable) | ✓ | ✓ | ✓ | ✓ |
| PWM8 (VC2, Period 255, Row_0_Output_2 → GlobalOutOdd_6 → P1[6]) | DBB01 | DBB01 | DBB01 | **DBB00** |
| UART (VC3) TX DCB02 / RX DCB03 | TX Row_0_Output_1 → GOE1 → P2[1]; RX P2[0] → GIE0 → Row_0_Input_0 | TX P1[4], RX P1[2] | como 24423A | como 24423A |
| DAC | DAC6 en ASD11 | DAC8 ASD11+ASC21 | DAC8 | DAC8 |
| Salida del DAC | AnalogOutBus_1 → **P0[5]** | igual | igual | igual |
| I2CHW maestro P1[5]/P1[7] | — | — | ✓ | ✓ |

### 16.3 Registros que comprueban que el proyecto está bien

| Registro | Valor | Significado |
|---|---|---|
| OSC_CR1 / OSC_CR4 / OSC_CR3 | 02h / 00h / 19h | VC1 = SysClk, VC2 = /3; VC3 = SysClk/26 |
| CLK_CR0 | 55h (o 05h con 2 columnas) | columnas analógicas en VC2 |
| RDI0SRO0 / RDI0SRO1 | 10h / 08h | TX → GOE1, PWM → GOO6 |
| PRT2GS / PRT1GS | 03h / 40h | P2[0], P2[1] y P1[6] en buses globales |
| DCB02IN / DCB03IN | 01h / C1h | UART con reloj VC3 |
| ACB00CR0 / ACB00CR1 | **FEh** / 21h | PGA ganancia 1, sin salida al bus analógico |
| ASC21CR2 | A0h | DAC8 |
| ABF_CR0 | 20h | buffer de la columna 1 → P0[5] |

> Las variantes 24423A/24223A de PsocDuino ya tienen el PGA corregido a ganancia 1
> (desde la v0.5.1), aunque los proyectos originales tenían ganancia 2.

---

## 17. Agregar otro chip (importar una variante)

1. PSoC Designer 5.4: **File → New Project → Chip-level**, elija el chip y lenguaje **C**.
2. **Global Resources**: `CPU_Clock` = SysClk/1, `Sleep_Timer` = **512_Hz**,
   `Power Setting` según su alimentación. Sin User Modules (o con los que quiera usar).
3. **Build → Generate** (o Build, F6).
4. Arrastre la carpeta del proyecto (la que contiene `project.mk`, `boot.asm` y `lib\`)
   sobre **`importar_variante.bat`** (carpeta de la plataforma instalada), o:
   ```
   importar_variante.bat "C:\ruta\al\proyecto" cy8c24794
   ```
5. Se guarda en `%LOCALAPPDATA%\PsocDuino\variants\<chip>` y **no se pierde al
   actualizar** PsocDuino.

`psocbuild import` copia `boot.asm`, `memory.inc` y `lib\`, lee `project.mk` y el
`linkfile`, calcula `F_CPU` y apunta el vector del Sleep Timer a `millis()`. Al final
ejecuta `psocbuild doctor`, que muestra dónde encontró `iccm8c`, `iasm8c`, `ilinkm8c` e
`ilibw`.

Más información: [docs/compilacion.md](docs/compilacion.md).

---

## 18. psocprog (programador por línea de órdenes)

Reimplementación multiplataforma de *PSoC Programmer CH341A v1.22*, hecha por
ingeniería inversa ([docs/ingenieria-inversa.md](docs/ingenieria-inversa.md)), con
mejoras:

- detección automática del chip por su ID de silicio (55 modelos), rechazo de IDs
  desconocidos y aviso si no coincide con la placa;
- verificación byte a byte y checksum corregido para todos los tamaños de flash;
- bits de seguridad escritos después de verificar;
- lectura de la flash a `.hex`, borrado, checksum, modos XRES/encendido, 5 V/3,3 V;
- el chip queda funcionando al terminar (`--run`).

```
psocprog -i                               # detectar el chip
psocprog -w programa.hex                  # borrar + programar + verificar
psocprog -c CY8C29466 -V 3.3 -w prog.hex  # exigir chip y programar a 3,3 V
psocprog -m power -w prog.hex             # entrada por encendido (POR)
psocprog -r copia.hex                     # leer la flash
psocprog -e                               # borrar
psocprog -k                               # checksum
psocprog --list                           # chips soportados
```

| Opción | Significado |
|---|---|
| `-c, --chip NOMBRE` | chip esperado (`auto` = cualquiera) |
| `-m, --mode xres\|power` | entrada a programación |
| `-V, --vdd 5\|3.3` | tensión |
| `-b, --backend auto\|dll\|libusb\|sim` | acceso al CH341A |
| `-n, --index N` | CH341A número N si hay varios |
| `--verify full\|checksum\|none` | verificación |
| `--no-secure` / `--protect` | sin protección / proteger todo |
| `--run` / `--hold` | al terminar: chip corriendo / en reset |
| `--force` | programar aunque el ID no coincida |
| `--retries N` | reintentos de conexión (3) |
| `--power-off-ms N` | tiempo sin Vdd en modo power (100) |
| `-q` / `-v` | silencioso / detallado |

En Windows usa la `CH341DLL.DLL` (32 bits) o `CH341DLLA64.DLL` (64 bits) del driver de
WCH; en Linux/macOS, libusb.

---

## 19. Solución de problemas

| Mensaje / síntoma | Causa y solución |
|---|---|
| `psocduino_perif.h: No such file or directory` / `la placa elegida no tiene perifericos` | usó `PsocDuinoPerif` con una placa sin periféricos (la línea `variante en uso:` dice cuál). Elija una placa **“+ perifericos”**, o en “PSoC 1 (todos los modelos)” el Microcontrolador **“... + perifericos”** (sección 5.1) |
| `la variante 'xxx' no esta importada` | chip sin variante: importe una (sección 17) |
| `no encuentro PSoC Designer` | defina `PSOC_DESIGNER_DIR` (sección 3.3) y reinicie el IDE |
| `cannot load source file "Designer\5.4\...\icppw.exe"` | Windows sin nombres cortos: `psocbuild` crea `%LOCALAPPDATA%\PsocDuino\psd`. Si no puede: `mklink /J %LOCALAPPDATA%\PsocDuino\psd "C:\Program Files (x86)\Cypress\PSoC Designer\5.4\Common\CypressSemiBuildMgr"` |
| `area 'xxx' not large enough` / el programa no cabe | el sketch es demasiado grande para el chip. El mensaje dice cuánto falta y deja el `.mp`. Vea la sección 13 o use un chip mayor |
| aviso `__xidata_start label not found` (chips LMM: 21x34, 29466) | inofensivo; el programa funciona |
| `error programando el bloque N: tiempo de espera agotado` | corregido en la v0.5.4: la espera de cada bloque es por tiempo (hasta 2 s), como el original. Actualice |
| `CH341A not present` | programador desconectado, driver no instalado o jumper fuera de 1–2 (sección 3.2) |
| `el PSoC no responde` | revise S1 y el menú *Entrada a programación* (POR/XRES), el cableado, la resistencia de 1 kΩ y la alimentación; nada conectado a P1_0/P1_1 |
| el ID leído no coincide con la placa | eligió otro chip en el IDE; `psocprog -i` muestra el real |
| `analogRead` siempre 0 | pata que no es P0_1/P0_3/P0_5/P0_7, o placa sin periféricos |
| `analogRead` llega al máximo con media tensión | versión anterior a 0.5.1 (PGA con ganancia 2): actualice |
| `analogWrite` solo enciende/apaga | la pata no es P1_6 |
| `Serial` pierde datos | velocidad alta con CPU lenta o lecturas poco frecuentes: baje a 9600/19200 |
| `delayMicroseconds` impreciso | calibre con **CalibrarDelay** (sección 12) |
| Windows bloquea `psocprog.exe`/`psocbuild.exe` | no tienen firma digital: *Más información → Ejecutar de todas formas* |
| error de instalación por privilegios (enlaces simbólicos) | actualice a la última versión del paquete |

---

## 20. Cómo funciona por dentro

```
sketch.ino ──psocbuild cc──► iccm8c (ImageCraft) ──► .o
núcleo (cores/psoc1) ───────► core.a (solo entra lo usado)
variante: boot.asm, lib/*.asm (PSoC Designer) ──► iasm8c ──► crtM8C.o, configuración,
                                                             libmodulos.a (User Modules)
todo ──psocbuild link──► ilinkm8c (opciones del linkfile de PSoC Designer) ──► .hex
.hex ──psocprog──► CH341A ──ISSP──► PSoC 1
```

- **`psocbuild`**: pasa el `.ino` a C89, usa rutas cortas de Windows, traduce los
  errores de ImageCraft al formato del IDE, revisa los `#include` para que el IDE
  encuentre las bibliotecas, ajusta `GlobalParams.inc` según los menús (tensión, CPU,
  Sleep Timer), ensambla la variante, enlaza con las mismas opciones que PSoC Designer y
  muestra flash/RAM desde el mapa `.mp`.
- **Variantes** (`variants/<chip>`): el `boot.asm`, `memory.inc` y `lib\` generados por
  PSoC Designer para ese chip; las “_perif” incluyen además los User Modules y
  `psocduino_perif.h`, que conecta la biblioteca con los nombres de cada módulo.
- **Núcleo** (`cores/psoc1`): C89 + rutinas en ensamblador (`psocduino_asm.S`) para
  registros y retardos exactos.

Detalles: [docs/compilacion.md](docs/compilacion.md), [docs/perifericos.md](docs/perifericos.md),
[docs/conexion.md](docs/conexion.md), [docs/ingenieria-inversa.md](docs/ingenieria-inversa.md).

---

## 21. Desarrollo y publicación

**Compilar las herramientas:**
```
cd tools/psocprog && make && make test     # programador + pruebas con simulador
cd tools/psocbuild && make                 # psocbuild
gcc -std=c89 -pedantic -DPSOCDUINO_HOST_TEST ... tests/core_host_test.c   # núcleo en el PC
```

**boards.txt** se genera con `python3 tools/packaging/gen_boards.py` (listas `BOARDS`,
`MENU_CHIPS` y `PERIF_BOARDS`). No lo edite a mano.

**Publicar una versión:** cambie `version=` en `platform.txt` y haga push a `main`.
GitHub Actions (`release.yml`) compila `psocprog`/`psocbuild` para Windows (x86/x64),
Linux y macOS, crea el Release `vX.Y.Z` con el paquete `.tar.bz2` y actualiza
`package_psocduino_index.json` (conserva las versiones anteriores).

**Licencia y créditos:** núcleo y herramientas de PsocDuino por CEPH81 /
MikrocontrollerCephs. PSoC Designer, ImageCraft y los User Modules son de Cypress
(Infineon); los archivos generados por PSoC Designer conservan su licencia.
