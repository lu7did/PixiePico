@echo off
setlocal
set "CMAKE_EXE=%USERPROFILE%\.pico-sdk\cmake\v4.3.4\bin\cmake.exe"
set "NINJA_EXE=%USERPROFILE%\.pico-sdk\ninja\v1.13.2\ninja.exe"

if not exist "%CMAKE_EXE%" (
    echo No se encontro CMake en "%CMAKE_EXE%"
    exit /b 1
)

if not exist build\build.ninja (
    "%CMAKE_EXE%" -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"
    if errorlevel 1 exit /b 1
)

"%CMAKE_EXE%" --build build --parallel
