@echo off
echo ===== WFS Client Library Automatic Build Script =====

:: Set vcpkg path (modify as needed)
set VCPKG_PATH=C:/dev/vcpkg

:: Create and enter build directory
if not exist build mkdir build
cd build

:: Configure project
echo Configuring project...
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake

:: Build project
echo Building project...
cmake --build . --config Release

:: Build completion message
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===== Build Successful =====
    echo Library file: .\build\Release\wfs_client.dll
    echo Example program: .\build\Release\wfs_client_example.exe
) else (
    echo.
    echo ===== Build Failed, Please Check Error Messages =====
)

:: Return to project root directory
cd ..

PAUSE 