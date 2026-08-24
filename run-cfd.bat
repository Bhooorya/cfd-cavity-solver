@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "EMCC_BAT=%USERPROFILE%\emsdk\upstream\emscripten\emcc.bat"
set "PORT=8000"

if not "%~1"=="" set "PORT=%~1"

if not exist "%EMCC_BAT%" (
  echo [ERROR] Emscripten compiler not found at:
  echo         %EMCC_BAT%
  echo Run emsdk install/activate first, then try again.
  exit /b 1
)

if not exist "%PROJECT_DIR%project-code.cpp" (
  echo [ERROR] project-code.cpp not found in:
  echo         %PROJECT_DIR%
  exit /b 1
)

if not exist "%PROJECT_DIR%project-webpage.html" (
  echo [ERROR] project-webpage.html not found in:
  echo         %PROJECT_DIR%
  exit /b 1
)

echo [1/3] Building solver.js + solver.wasm ...
call "%EMCC_BAT%" "%PROJECT_DIR%project-code.cpp" -O3 --bind -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME=createCavityModule -s ALLOW_MEMORY_GROWTH=1 -o "%PROJECT_DIR%solver.js"
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo [2/3] Opening browser ...
start "" "http://127.0.0.1:%PORT%/project-webpage.html"

echo [3/3] Starting local server on port %PORT% ...
cd /d "%PROJECT_DIR%"
python -m http.server %PORT%

endlocal