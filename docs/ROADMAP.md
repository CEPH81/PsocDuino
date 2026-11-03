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

## Fase 3 — Periféricos (`cores/psoc1`)
- [ ] PWM, interrupciones por pin (`attachInterrupt`)
- [ ] Ejemplos y paquete para el Gestor de placas (`package_psocduino_index.json`)
