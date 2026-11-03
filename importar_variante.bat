@echo off
setlocal
rem PsocDuino - importa la configuracion de un proyecto de PSoC Designer.
rem Uso: arrastre la carpeta del proyecto sobre este archivo, o:
rem      importar_variante.bat "C:\ruta\al\proyecto" cy8c27443
rem La variante se guarda en %LOCALAPPDATA%\PsocDuino\variants\<chip>
rem (se conserva al actualizar PsocDuino desde el Gestor de placas).
set "PROY=%~1"
set "CHIP=%~2"
if "%PROY%"=="" set /p "PROY=Carpeta del proyecto de PSoC Designer (con project.mk): "
if "%CHIP%"=="" (
  echo Chips: cy8c24423a  cy8c27443  cy8c29466
  set /p "CHIP=Chip: "
)
"%~dp0tools\bin\windows\psocbuild.exe" import "%PROY%" %CHIP%
if errorlevel 1 goto fin
"%~dp0tools\bin\windows\psocbuild.exe" doctor -V %CHIP%
:fin
echo.
pause
