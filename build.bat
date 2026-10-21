@echo off
echo ==============================================================
echo Building ARINC 429 Avionics Protocol Analyzer
echo Author: Amit Bakale (Avionics / Embedded Software Engineer)
echo ==============================================================
gcc -O2 -Wall -Iinclude src\arinc429.c src\main.c -o arinc429_analyzer.exe
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed successfully: arinc429_analyzer.exe
    echo Run arinc429_analyzer.exe to execute telemetry decoding and fault injection!
) else (
    echo [FAILED] Compilation failed with error %ERRORLEVEL%
)
