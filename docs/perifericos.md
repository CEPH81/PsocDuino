# Periféricos: Serial, analogRead, analogWrite y dacWrite

Las placas **"PSoC 1 CY8C24423A / CY8C24223A / CY8C27443 / CY8C29466 + periféricos"**
usan los User Modules de los proyectos `Base2_cy8c24423A`, `Base2_cy8c24223A`,
`Base2_cy8c27443` y `Base2_cy8c29466` de PSoC Designer. Las demás placas no cambian
(no tienen periféricos).

```c
#include <PsocDuinoPerif.h>
```

| Función | CY8C24423A | CY8C24223A | CY8C27443 / CY8C29466 | Detalles |
|---|---|---|---|---|
| `Serial` (UART, 8N1) | TX **P2_1**, RX **P2_0** | TX **P1_4**, RX **P1_2** | TX **P2_1**, RX **P2_0** | 115200 por defecto; también 57600, 38400, 19200, 9600, 4800 |
| `analogRead(pin)` | P0_1, P0_3, P0_7 | igual | igual | ADCINC de 12 bits, PGA con ganancia 1 (0..Vdd); devuelve 0..1023 (`analogReadResolution(6..12)`) |
| `analogWrite(P1_6, v)` | P1_6 | P1_6 | P1_6 | PWM8 por hardware, 0..255 a 31,25 kHz. Otras patas: alto si v ≥ 128 |
| `dacWrite(v)` | P0_5 (DAC6, 64 escalones) | P0_5 (DAC8, 256 escalones) | P0_5 (DAC8) | 0..255 → 0 V..Vdd |
| I2C por hardware | — | — | SDA **P1_5**, SCL **P1_7** | Módulo I2CHW maestro: `#include "I2CHW_1Mstr.h"` y funciones `I2CHW_1_...` de PSoC Designer |

En el CY8C27443 y el CY8C29466 las filas digitales 1 a 3 quedan libres (más PWM, timers o
SPI en el futuro). En el CY8C29466 el PWM8 está en DBB00 y el contador del ADC en DBB01;
funciona igual.

`Serial` es una estructura de C (ImageCraft no compila C++), así que:

```c
Serial.begin(115200);
Serial.print("texto");      /* solo textos entre comillas (van en la flash) */
Serial.println("texto");
Serial.printNum(n);          /* numeros: printNum / printlnNum (long) */
Serial.printlnNum(n);
Serial.write(c);
if (Serial.available()) c = Serial.read();
```

El UART no tiene búfer de recepción por interrupción: `available()` guarda **un** byte.
A 115200 lea con frecuencia (o use una velocidad menor) si el PC envía muchos datos seguidos.

**Memoria:** las rutinas de los User Modules se enlazan desde una biblioteca, así que solo
ocupan flash las que usa el sketch (las de interrupción y la configuración siempre entran).

Ejemplos: *Archivo → Ejemplos → PsocDuinoPerif → Serial_Eco, Potenciometro_ADC, PWM_Fade,
DAC_Rampa*.

## Cómo se hizo la configuración (para otros chips)

| Recurso | Bloque | Reloj |
|---|---|---|
| ADCINC (contador) | DBB00 | VC2 = 8 MHz (Data Clock), columnas analógicas en VC2 |
| PWM8 | DBB01 | VC2 → Row_0_Output_2 → GlobalOutOdd_6 → P1[6] |
| UART TX / RX | DCB02 / DCB03 | VC3 = SysClk/26 |
| PGA + ADCINC | ACB00, ASC10, ASD20 | columna 0 |
| DAC6 / DAC8 | ASD11 (+ASC21) | columna 1, salida AnalogOutBus_1 → P0[5] |

Global Resources: VC1 = SysClk/1, VC2 = VC1/3, VC3 = SysClk/26, Sleep Timer 512 Hz.
