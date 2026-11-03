# PsocDuino

Núcleo Arduino para microcontroladores **Cypress PSoC 1** (CY8C24423A, CY8C27443,
CY8C29466 y 52 más), programados con un **CH341A** por ISSP.

Proyecto de [MikrocontrollerCephs](https://www.youtube.com/@mikrocontrollersceph).

## Estado

| Parte | Estado |
|---|---|
| `psocprog`: programador CH341A (Windows / Linux / macOS) | ✅ listo, probado en simulador |
| Subida desde el Arduino IDE (`platform.txt`, `boards.txt`) | ✅ |
| Compilación con ImageCraft vía `psocbuild` ([docs/compilacion.md](docs/compilacion.md)) | ✅ probado con arduino-cli; falta confirmar con ImageCraft real |
| API básica: pines, `millis`, `delay`, `shiftOut`, `pulseIn`… | ✅ |
| `Serial`, `analogRead`, PWM (módulos de usuario) | 🔜 fase 3 |

Ver [docs/ROADMAP.md](docs/ROADMAP.md).

## psocprog

Reimplementación multiplataforma de *PSoC Programmer CH341A v1.22*, obtenida por
ingeniería inversa ([docs/ingenieria-inversa.md](docs/ingenieria-inversa.md)), con
mejoras:

- **Detección automática** del chip por su ID de silicio (55 modelos); rechaza
  IDs desconocidos y avisa si no coincide con la placa elegida.
- Verificación **byte a byte** tras programar + checksum corregido para todos los
  tamaños de flash.
- Bits de seguridad del `.hex` escritos después de verificar (o `--no-secure`).
- Lectura de la flash a `.hex`, borrado, checksum, modo XRES o encendido, 5 V / 3,3 V.
- El chip queda funcionando al terminar (`--run`).
- Windows usa la misma `CH341DLL.DLL` y driver del programa original; Linux/macOS usan libusb.

```
psocprog -i                               # detectar el chip
psocprog -w programa.hex                  # borrar + programar + verificar
psocprog -c CY8C29466 -V 3.3 -w prog.hex  # exigir chip y programar a 3,3 V
psocprog -r copia.hex                     # leer la flash
psocprog --list                           # chips soportados
```

Conexión: [docs/conexion.md](docs/conexion.md) (zócalo SPI del CH341A: CS→XRES,
CLK→SCLK, MOSI–1 kΩ–SDATA–MISO).

### Compilar

```
cd tools/psocprog
make            # Linux/macOS (libusb-1.0)
make test       # pruebas con el simulador CH341A+PSoC integrado
```
Los binarios para Windows, Linux y macOS los genera GitHub Actions
(`.github/workflows/psocprog.yml`); se copian en `tools/bin/<so>/`.

En Linux instale `tools/udev/99-ch341a-psocduino.rules`. En Windows ponga junto a
`psocprog.exe` la `CH341DLL.DLL` (versión de 32 bits) o la `CH341DLLA64.DLL` (versión de 64 bits) del driver CH341PAR de WCH.

## Instalación manual en el Arduino IDE 2.x (Windows)

1. Clone o copie este repositorio en `Documentos\Arduino\hardware\psocduino\m8c\`.
2. Descargue el artefacto `psocduino-tools-windows-x86` de GitHub Actions y copie
   `psocprog.exe` y `psocbuild.exe` en `tools\bin\windows\`, junto con la
   `CH341DLL.DLL` del programa original.
3. Importe una variante por chip desde un proyecto vacío de PSoC Designer
   ([docs/compilacion.md](docs/compilacion.md)):
   `importar_variante.bat "C:\...\base_cy8c27443\base_cy8c27443" cy8c27443`
4. Reinicie el IDE: aparecen las placas *PSoC 1* con los menús de entrada
   (POR/XRES), tensión, seguridad y verificación, y los ejemplos *PsocDuino*.
   **Quemar bootloader** borra el chip.
