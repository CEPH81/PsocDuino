# Instalación desde el Gestor de placas (Arduino IDE 2.x)

## 1. Agregar el índice de PsocDuino

1. **Archivo → Preferencias**.
2. En *URLs adicionales de gestor de placas* agregue (una por línea):
   ```
   https://raw.githubusercontent.com/CEPH81/PsocDuino/main/package_psocduino_index.json
   ```
3. Aceptar.

## 2. Instalar la plataforma

1. **Herramientas → Placa → Gestor de placas** (o el ícono de placas a la izquierda).
2. Busque **PsocDuino** y pulse **Instalar**.

Se instala en `%LOCALAPPDATA%\Arduino15\packages\psocduino\hardware\m8c\<versión>\`
con todo incluido: núcleo, ejemplos, `psocprog.exe`, `psocbuild.exe` y `CH341DLL.DLL`.

## 3. Driver del CH341A (una vez)

Instale el driver de la carpeta `CH341A driver\CH341_EPP_MEM_I2C_IO\` del
programa original (clic derecho en `CH341WDM.INF` → Instalar), o el paquete
**CH341PAR** de WCH. El jumper del CH341A Mini debe estar en **1–2**.

## 4. Requisito para compilar: PSoC Designer 5.4

PsocDuino usa el compilador ImageCraft que instala PSoC Designer. Por cada
chip, una sola vez, importe su variante (ver [compilacion.md](compilacion.md)):

1. Cree en PSoC Designer un proyecto vacío en C para el chip y compílelo (F6).
2. Abra la carpeta de la plataforma instalada (paso 2) y **arrastre la carpeta
   del proyecto** (la que contiene `project.mk`) sobre **`importar_variante.bat`**;
   escriba el chip (`cy8c24423a`, `cy8c27443` o `cy8c29466`).

La variante se guarda en `%LOCALAPPDATA%\PsocDuino\variants\<chip>`, fuera de la
plataforma, así que **no se pierde al actualizar** PsocDuino.

## 5. Compilar y subir

1. **Herramientas → Placa → PsocDuino** → su chip.
2. Menús en **Herramientas**:
   - *Entrada a programación*: **Encendido / POR** (circuito con Q1 y S1 en POR)
     o **XRES**;
   - *Tensión de programación*: 5 V (también en modo POR) o 3,3 V;
   - *Seguridad de flash*: sin protección / según el `.hex` / proteger todo;
   - *Verificación*: completa o rápida.
3. **Herramientas → Programador → CH341A (psocprog, ISSP)**.
4. **Subir** (no hace falta elegir puerto). *Quemar bootloader* borra el chip.

Ejemplos: **Archivo → Ejemplos → PsocDuino → Blink / Boton / CalibrarDelay**.

## Solución de problemas

| Mensaje | Qué hacer |
|---|---|
| `la variante 'cy8c27443' no esta importada` | Paso 4 |
| `no encuentro PSoC Designer` | Defina la variable de entorno `PSOC_DESIGNER_DIR` con la ruta de `...\PSoC Designer\5.4\Common\CypressSemiBuildMgr` y reinicie el IDE |
| `CH341A not present` | Programador desconectado o driver no instalado (paso 3) |
| `el PSoC no responde` | Revise S1 y el menú de entrada (POR/XRES), cableado y alimentación |
| Windows bloquea `psocprog.exe` | Son programas sin firma digital: *Más información → Ejecutar de todas formas* |

Para ver las órdenes completas active en **Preferencias** *Mostrar salida detallada*
para compilar y subir.
