# Conexión CH341A → PSoC 1

Se usa el conector SPI de 8 pines (zócalo 25xx) del programador CH341A "mini":

| CH341A (zócalo 25xx) | Pin CH341 | PSoC 1 | Notas |
|---|---|---|---|
| 1 CS | D0 | **XRES** | modo `xres` (por defecto) |
| 6 CLK | D3 | **SCLK** = P1[1] | |
| 5 MOSI | D5 | **SDATA** = P1[0] vía **1 kΩ** | el programador escribe |
| 2 MISO | D7 | **SDATA** = P1[0] directo | el programador lee |
| 4 GND | GND | Vss | |
| 8 VCC | 5 V / 3,3 V | Vdd | según el jumper del CH341A |

```
 CH341A                       PSoC 1
 MOSI (D5) ──[ 1 kΩ ]──┬───── SDATA  P1[0]
 MISO (D7) ────────────┘
 CLK  (D3) ────────────────── SCLK   P1[1]
 CS   (D0) ────────────────── XRES
 GND       ────────────────── Vss
 VCC       ────────────────── Vdd
```

**Tensión:** si el jumper del CH341A está en 5 V use `-V 5` (menú "5 V"); en 3,3 V
use `-V 3.3`. La tensión de los pines del CH341A debe coincidir con la Vdd del PSoC.

**Modo encendido** (`-m power`, chips sin XRES o placas donde XRES no está
accesible): D0 controla la Vdd del PSoC mediante un transistor PNP (D0 = 0 →
alimentado). psocprog apaga Vdd `--power-off-ms` ms (100 por defecto), la
enciende y entra en programación en los primeros microsegundos.

Mantenga los cables cortos (< 15 cm). Si no detecta el chip, pruebe primero
`psocprog -i` y revise que P1[0]/P1[1] no tengan cargas fuertes en la placa.
