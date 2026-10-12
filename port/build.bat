@echo off
rem Builds the raylib port with the Visual Studio toolchain (cmake/ninja/cl are not on PATH by default).
rem Usage: build.bat [extra cmake --build args]
set "VS=C:\Program Files\Microsoft Visual Studio\18\Community"
set "PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\Installer"
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "CMK=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%PATH%;%CMK%\CMake\bin;%CMK%\Ninja"
cd /d "%~dp0"
if not exist build cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build %*
