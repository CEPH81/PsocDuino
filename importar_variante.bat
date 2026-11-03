@echo off
rem PsocDuino - importa la configuracion de un proyecto de PSoC Designer como variante.
rem Uso:  importar_variante.bat "C:\ruta\al\proyecto" cy8c27443
rem (la carpeta del proyecto es la que contiene project.mk, boot.asm y lib\;
rem  compile el proyecto en PSoC Designer antes de importarlo)
if "%~2"=="" (
  echo Uso: importar_variante.bat "C:\ruta\al\proyecto" cy8c24423a^|cy8c27443^|cy8c29466
  exit /b 2
)
"%~dp0tools\bin\windows\psocbuild.exe" import "%~1" "%~dp0variants\%~2"
if errorlevel 1 exit /b 1
"%~dp0tools\bin\windows\psocbuild.exe" doctor -V "%~dp0variants\%~2"
