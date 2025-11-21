@echo off
REM Batch build script for the consumer example project

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set ORION_ROOT=%SCRIPT_DIR%..\..

echo === Building Orion Consumer Example ===
echo Orion root: %ORION_ROOT%
echo Example dir: %SCRIPT_DIR%

REM Check if vcpkg is available
if "%VCPKG_ROOT%"=="" (
    echo Error: VCPKG_ROOT environment variable not set
    echo Please set it to your vcpkg installation directory:
    echo   set VCPKG_ROOT=C:\path\to\vcpkg
    exit /b 1
)

set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
if not exist "%VCPKG_TOOLCHAIN%" (
    echo Error: vcpkg toolchain file not found at %VCPKG_ROOT%
    exit /b 1
)

echo Using vcpkg at: %VCPKG_ROOT%

REM Configure with vcpkg overlay pointing to orion ports
echo.
echo === Configuring project ===

set BUILD_DIR=%SCRIPT_DIR%build
set OVERLAY_PORTS=%ORION_ROOT%\ports

cmake -B "%BUILD_DIR%" -S "%SCRIPT_DIR%" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" ^
    -DVCPKG_OVERLAY_PORTS="%OVERLAY_PORTS%" ^
    -DVCPKG_FEATURE_FLAGS=-binarycaching ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo Configuration failed
    exit /b %errorlevel%
)

REM Build
echo.
echo === Building project ===
cmake --build "%BUILD_DIR%" --config Release

if errorlevel 1 (
    echo Build failed
    exit /b %errorlevel%
)

echo.
echo === Build complete ===
echo Executable: %BUILD_DIR%\Release\consumer-example.exe
echo.
echo Run with:
echo   cd %SCRIPT_DIR%
echo   .\build\Release\consumer-example.exe

endlocal
