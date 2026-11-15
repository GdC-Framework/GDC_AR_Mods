@echo off
rem Drag a console.log (or its logs folder) onto this file: writes the report next to the log and opens it.
chcp 65001 >nul
if "%~1"=="" (
  echo Glisser un console.log ^(ou son dossier logs_...^) sur ce fichier.
  pause
  exit /b 1
)
python -I "%~dp0server_log_report.py" "%~1" --open
if errorlevel 1 pause
