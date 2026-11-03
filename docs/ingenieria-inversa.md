# Ingeniería inversa de "PSoC Programmer CH341A v1.22"

Origen: `PSoC_Programmer v1_22.exe` (antiradio.narod.ru/psoc), Borland C++ Builder,
32 bits, con `CH341DLL.DLL` (WCH, 2008). Se analizaron el ejecutable, la DLL y el
formulario VCL (DFM) para reconstruir el protocolo; **psocprog** es una
reimplementación nueva, no una copia del código.

## 1. Capa CH341A

El programa usa solo 8 funciones de la DLL:

| Función DLL | Uso | Comando USB equivalente |
|---|---|---|
| `CH341OpenDevice/CloseDevice` | abrir/cerrar | — |
| `CH341SetStream(0, 0x80)` | SPI MSB-first, velocidad 0 | `AA 60 00` |
| `CH341Set_D5_D0(0, 0x29, x)` | D0, D3, D5 como salidas | `AB 80\|data 40\|dir 20` |
| `CH341SetOutput(0, 0x1F, 1, x)` | al terminar: solo D0 salida | igual (UIO) |
| `CH341GetInput` | leer SDATA (bit 7 = D7) | `A0` |
| `CH341BitStreamSPI` | vectores ISSP bit a bit | `AB` + por bit: `80\|v`, `88\|v` (SCLK↑), `00` (leer), `80\|v`; 7 bits/paquete, `20` |
| `CH341StreamSPI4` | 64 WRITE-BYTE / READ-BYTE por bloque | `A8` + 31 bytes, bits invertidos (el CH341 es LSB-first) |

Pines: **D0** = XRES (o control de Vdd), **D3** = SCLK, **D5** = SDATA salida,
**D7** = SDATA entrada. Son los pines CS, CLK, MOSI y MISO del zócalo SPI del CH341A.

## 2. Vectores ISSP

Guardados en el .exe como texto `"0"/"1"` (cada `1` → byte `0x20` = D5). Los 23
vectores coinciden con los de Cypress (AN2026). Decodificados con la gramática ISSP
(`100 a8 d8 111` = escribir SRAM, `110 a8 d8 111` = escribir registro,
`101 a8 Z d8 Z1` = leer SRAM; `R[FF]=12` ejecuta la función SSC de `R[F0]`):

| Vector | SSC | Notas |
|---|---|---|
| INIT-1/2, INIT-3 (3,3 V / 5 V), ID-SETUP | 9, 6 | entrada a modo programación |
| READ-ID | — | lee `M[F8]`, `M[F9]` |
| ERASE | 5 (EraseAll) | |
| PROGRAM-BLOCK ×3 familias | 2 | `M[FC]` (CLOCK) = 0x54 o 0x15 |
| VERIFY-SETUP | 1 (ReadBlock) | |
| SECURE ×3 | 4 | |
| CHECKSUM-SETUP ×5 | 7 | `M[FA]` = nº de bloques |
| SET-BANK / SET-BLOCK-NUM | — | `R[FA]` con XIO / `M[FA]` |

Tiempos: tras cada vector el programa añade N relojes a 0 y comprueba que SDATA
suba 2 relojes después (ocupado) y luego baje (WAIT-AND-POLL), o sondea
`GetInput` hasta 500 veces. Los datos leídos aparecen en las posiciones 13–20 de
cada palabra (un reloj después que en la especificación) por el instante de
muestreo del CH341.

## 3. Tabla de chips

55 chips, reconocidos por los 2 bytes del ID de silicio: tamaño (KB), bancos y
qué vectores de familia usar. Bloques por banco = 128 si hay ≥ 2 bancos, si no
`KB × 16`. Ver `tools/psocprog/src/vectors.c` (`psocprog --list`).

### Cómo se conoce el ID de silicio

Cada PSoC 1 trae grabado de fábrica, en la ROM de supervisión (tabla 0), un ID
de 2 bytes por modelo. El programador lo lee así:

1. **ID-SETUP** ejecuta la función SSC 6 (*TableRead*, `R[F0]=06`, tabla `M[FA]=00`),
   que copia el ID a la SRAM `F8h`–`F9h`.
2. **READ-ID-WORD** lee esas dos posiciones (`101 11111000 Z dddddddd Z1` y
   `101 11111001 …`).

Los valores los publica Cypress en sus *PSoC 1 ISSP Programming Specifications*
(por ejemplo CY8C29466 = `002A`, CY8C29566 = `002B`, CY8C24794 = `001D`, que
coinciden con la tabla del programa original). Para un chip nuevo: `psocprog -i`
muestra el ID leído aunque no esté en la tabla.

## 4. Problemas del original corregidos en psocprog

1. **ID desconocido → se programaba como CY8C27xxx de 16 K.** Ahora es un error
   (se puede forzar con `--chip X --force`).
2. **Checksum incompleto:** CHECKSUM-SETUP llevaba un nº de bloques fijo por
   familia (32, 64 o 128). En CY8C21312/21345/21512/21645 (8 K), CY8C22345/22545/
   22645 (16 K) y otros solo se sumaba parte de la flash. psocprog escribe el nº
   real de bloques por banco.
3. Verificación solo bajo demanda → ahora se verifica byte a byte después de
   programar y **antes** de escribir la protección (si no, un `.hex` con
   protección "W" impediría verificar).
4. En modo encendido no se esperaba con Vdd apagada → `--power-off-ms`.
5. Al terminar el chip quedaba en reset → `--run` (por defecto) lo deja corriendo.
6. Solo Windows y una GUI → CLI para Windows (misma DLL y driver), Linux y macOS
   (libusb), integrable en el Arduino IDE.

## 5. Pendiente de confirmar con hardware

- Datos de la tabla heredados del original sin verificar contra hojas de datos:
  CY8C23x33 (8 K con 2 bancos) y las variantes con sufijo (-12xxXE, -24xxXA).
- Valor de la resistencia serie en SDATA (no figura en el programa; se
  recomienda 1 kΩ).
