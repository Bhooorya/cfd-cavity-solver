@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "PORT=8000"
if not "%~1"=="" set "PORT=%~1"

echo Opening CFD app on http://127.0.0.1:%PORT%/project-webpage.html
start "" "http://127.0.0.1:%PORT%/project-webpage.html"

cd /d "%PROJECT_DIR%"

where python >nul 2>nul
if %errorlevel%==0 (
  python -m http.server %PORT%
  goto :eof
)

where py >nul 2>nul
if %errorlevel%==0 (
  py -m http.server %PORT%
  goto :eof
)

echo.
echo [ERROR] Python was not found on this machine.
echo Install Python from https://www.python.org/downloads/ and run this file again.
pause
exit /b 1
