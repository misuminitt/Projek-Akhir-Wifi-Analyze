@echo off
cd /d "%~dp0"

g++ -std=c++98 -Wall -Wextra -pedantic -static -static-libgcc -static-libstdc++ wifi_analyze_windows.cpp -o wifi_analyze_windows.exe

if errorlevel 1 (
    echo Build gagal.
    exit /b 1
)

echo Build selesai: Windows\wifi_analyze_windows.exe
