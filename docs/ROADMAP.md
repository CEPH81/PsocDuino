# Hoja de ruta PsocDuino

## Fase 1 — Programador ✅
- [x] Ingeniería inversa de PSoC Programmer CH341A v1.22 (ver `ingenieria-inversa.md`)
- [x] `psocprog` multiplataforma: detectar, borrar, programar, verificar, leer,
      checksum, bits de seguridad; 55 chips con detección automática por ID
- [x] Simulador CH341A + PSoC y batería de pruebas (`make test`)
- [x] `boards.txt` / `platform.txt`: subida, borrado ("Quemar bootloader"), menús
- [x] Binarios por GitHub Actions (Windows x86/x64, Linux, macOS)
- [ ] Prueba con hardware real: CY8C24423A, CY8C27443, CY8C29466

## Fase 2 — Compilación con ImageCraft (PSoC Designer 5.4)
Necesario de Carlos: un proyecto vacío en C por chip, compilado en PSoC Designer,
con su log de compilación (`output/`, `lib/`, `boot.asm`, `m8c.inc`, `.hex`).
- [ ] Recetas reales de `iccm8c` / ensamblador / enlazador en `platform.txt`
- [ ] Arranque (`boot.asm`) y configuración global por variante

## Fase 3 — API Arduino (`cores/psoc1`)
- [ ] `pinMode`, `digitalWrite`, `digitalRead` (registros PRTxDR / PRTxDMx)
- [ ] `millis`, `delay`, `delayMicroseconds` (Sleep Timer / contador)
- [ ] `Serial` (bloque digital UART), `analogRead` (ADC en bloques analógicos)
- [ ] Ejemplos y paquete para el Gestor de placas (`package_psocduino_index.json`)
