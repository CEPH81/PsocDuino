# Compilar sketches con ImageCraft (PSoC Designer)

PsocDuino usa el compilador **ImageCraft ICCM8C** que instala PSoC Designer 5.4
(gratuito). Entre el IDE y ImageCraft está `psocbuild`, que:

- convierte el sketch `.ino`/`.cpp` en `.c` (ImageCraft solo compila **C**);
- usa rutas cortas de Windows, como PSoC Designer;
- traduce los errores `!E archivo(línea):` al formato del IDE (se resaltan en el editor);
- ensambla `boot.asm` y enlaza con **las mismas opciones que PSoC Designer** usó
  para ese chip (`linkfile`, `libpsoc.a`, `cms.a`, `arith.a`);
- muestra el uso de flash y RAM a partir del mapa (`.mp`).

## 1. Crear una variante por chip (una sola vez)

La configuración que depende del chip (arranque, oscilador, tensión, calibración,
memoria) la genera PSoC Designer. Para cada chip:

1. En PSoC Designer 5.4: **File → New Project → Chip-level**, nombre por ejemplo
   `base_cy8c27443`, elija el chip (CY8C24423A, CY8C27443 o CY8C29466, cualquier
   encapsulado) y lenguaje **C**.
2. En **Global Resources**:
   - `CPU_Clock` = **SysClk/1** (24 MHz) a 5 V, o SysClk/2 (12 MHz) a 3,3 V;
   - `Sleep_Timer` = **512_Hz** (PsocDuino lo usa para `millis()`);
   - `Power Setting [Vcc / SysClk freq]` según su alimentación (5.0V / 24MHz);
   - deje el resto por defecto y **no agregue módulos de usuario**.
3. **Build → Build** (F6). Debe terminar sin errores.
4. Importe la variante desde la carpeta del núcleo:

```
importar_variante.bat "C:\Users\...\base_cy8c27443\base_cy8c27443" cy8c27443
```

La carpeta correcta es la que contiene `project.mk`, `boot.asm` y `lib\`.
`psocbuild import` copia `boot.asm`, `memory.inc`, `lib\` (con `libpsoc.a`), lee
`project.mk` y `linkfile`, calcula `F_CPU` desde `GlobalParams.inc` y apunta el
vector del Sleep Timer (`64h`) a `_psocduino_sleep_isr`. Al final ejecuta
`psocbuild doctor`, que muestra dónde encontró `iccm8c`, `iasm8c`, `ilinkm8c` e `ilibw`.

Si PSoC Designer está en otra carpeta, defina la variable de entorno
`PSOC_DESIGNER_DIR` con la ruta de `...\PSoC Designer\5.4\Common\CypressSemiBuildMgr`.

### Variantes incluidas y menús

PsocDuino trae las variantes de CY8C21234, CY8C21534, CY8C24423A y CY8C27x43
(importadas de los proyectos de Carlos). `psocbuild import` también acepta
proyectos **solo generados** (sin compilar): toma `opts.txt` y `PSoCConfig.xml`,
reconstruye `memory.inc` y en cada compilación:

- reescribe `GlobalParams.inc` según los menús **Alimentación** (5 V / 3,3 V:
  `POWER_SETTING` o `SUPPLY_VOLTAGE`, y LVD) y **Reloj de CPU** (`CPU_CLOCK`);
  `boot.asm` aplica la calibración del IMO y del bandgap para esa tensión;
- ensambla `boot.asm` y `lib\*.asm` (configuración global, en lugar de `libpsoc.a`);
- busca en PSoC Designer la carpeta de `m8c.inc`, la `cms.a` de la familia y la
  `arith.a` del modelo de memoria (SMM/LMM, con o sin MAC). `psocbuild doctor -V <chip>`
  muestra qué encontró.

## 2. Compilar y subir

En el Arduino IDE elija la placa *PSoC 1 CY8C…*, los menús (entrada POR/XRES,
5 V/3,3 V, seguridad, verificación) y pulse **Subir**. Ejemplos en
*Archivo → Ejemplos → PsocDuino*.

## API disponible

`pinMode` (INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT_OPEN_DRAIN,
INPUT_ANALOG), `digitalWrite`, `digitalRead`, `digitalToggle`, `millis`, `micros`,
`delay`, `delayMicroseconds`, `shiftIn/Out`, `pulseIn`, `map`, `random`,
`randomRange`, `interrupts`/`noInterrupts`, macros de bits. Pines `P0_0`…`P5_7`
(P1_0/P1_1 son SDATA/SCLK de programación; se pueden usar después).

Limitaciones de ImageCraft: solo C89 (sin clases, sin sobrecarga: `random(a, b)`
es `randomRange(a, b)`; declare las variables al inicio de cada bloque).
`Serial` y `analogRead` llegarán con módulos de usuario (fase 3).

## Tiempos

- `delay`/`delayMicroseconds`: bucles de ciclos sobre el IMO (±2,5 %). El
  costo de llamada se ajusta con `PSOCDUINO_CALL_CYCLES` (por defecto 60).
  Mídalo con el ejemplo **CalibrarDelay** y el osciloscopio.
- `millis`: Sleep Timer a 512 Hz desde el ILO (15–64 kHz). Al arrancar se mide
  un tic contra el reloj de CPU y se corrige, así que es preciso aunque el ILO no lo sea.

## Pendiente de confirmar en Windows

Probado con arduino-cli y herramientas simuladas: preprocesado, compilación del
sketch y del núcleo (validados como C89), biblioteca, `boot.asm`, enlace y tamaño.
Falta confirmarlo con el ImageCraft real:
- que `iccm8c` acepte `-c -e -o` y las `-D`/`-I` tal como se pasan (ver `psocbuild -v`);
- los nombres de las carpetas de bibliotecas de PSoC Designer (se buscan solas;
  si `doctor` no las encuentra, envíe `dir /x` de `tools\include` y `tools\lib`).
