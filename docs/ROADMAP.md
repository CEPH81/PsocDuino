# Hoja de ruta PsocDuino

## Fase 1 — Programador ✅
- [x] Ingeniería inversa de PSoC Programmer CH341A v1.22 (ver `ingenieria-inversa.md`)
- [x] `psocprog` multiplataforma: detectar, borrar, programar, verificar, leer,
      checksum, bits de seguridad; 55 chips con detección automática por ID
- [x] Simulador CH341A + PSoC y batería de pruebas (`make test`)
- [x] `boards.txt` / `platform.txt`: subida, borrado ("Quemar bootloader"), menús
- [x] Binarios por GitHub Actions (Windows x86/x64, Linux, macOS)
- [ ] Prueba con hardware real: CY8C24423A, CY8C27443, CY8C29466

## Fase 2 — Compilación con ImageCraft (PSoC Designer 5.4) ✅
- [x] `psocbuild`: .ino→C, rutas cortas, errores al formato del IDE, enlace con
      el `linkfile` de PSoC Designer, tamaño desde el `.mp`
- [x] `psocbuild import` + `importar_variante.bat` (variante desde un proyecto vacío)
- [x] Núcleo C89: pines, `millis` (Sleep Timer calibrado), `delay`, `shiftOut`, `pulseIn`
- [x] Probado con arduino-cli y herramientas ImageCraft simuladas
- [ ] Confirmar con ImageCraft real en Windows (CY8C24423A, CY8C27443, CY8C29466)
- [ ] Calibrar `PSOCDUINO_CALL_CYCLES` con el osciloscopio (ejemplo CalibrarDelay)

## Fase 3 — Periféricos
- [x] Paquete para el Gestor de placas (`package_psocduino_index.json`)
- [x] Placas "+ perifericos" (CY8C24423A, CY8C24223A, CY8C27443, CY8C29466) y biblioteca
      PsocDuinoPerif: Serial, analogRead, analogWrite (PWM8), dacWrite
- [x] I2C por hardware (I2CHW) en 27443/29466; I2C por software en todos
- [x] Ejemplos: LCD, 74HC595, EEPROM 24C256, MPR121, Serial, ADC, PWM, DAC
- [ ] Probar los periféricos en el chip
- [ ] SPI por hardware (SPIM en la fila 1 de 27443/29466) y biblioteca `SPI`
- [ ] Biblioteca `Wire` sobre I2CHW
- [ ] Más canales PWM, `attachInterrupt`
