@echo off
rem starts the locally built hex grid prototype at the main menu.
rem The game expects its executable next to the maps and resources folders, so a newer build is copied here first.
set "PATH=C:\msys64\mingw64\bin;%PATH%"
cd /d "%~dp0"
if exist "build\Commander_Wars.exe" xcopy /D /Y /Q "build\Commander_Wars.exe" "." >nul 2>&1
start "" "Commander_Wars.exe"
