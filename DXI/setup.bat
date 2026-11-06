@echo off
rem ---------------------------------------------------------------------------
rem  setup.bat | Configures the CMake build tree in build\
rem
rem  Safe to re-run: CMake reuses the existing cache and only regenerates what
rem  changed. Run this after editing CMakeLists.txt, or after adding a file when
rem  build.bat still cannot see it.
rem
rem  Extra arguments are passed straight through, for example:
rem      setup.bat -DUSE_PIX=OFF
rem      setup.bat -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
rem ---------------------------------------------------------------------------

setlocal
pushd "%~dp0"

call :FindCMake
if errorlevel 1 goto :Failed

rem Models are not in the repository, see download_assets.bat for why
call "%~dp0download_assets.bat"
if errorlevel 1 goto :Failed
echo.

echo Configuring DXIApp into "%~dp0build"
echo.
"%CMAKE%" -S . -B build %*
if errorlevel 1 goto :Failed

echo.
echo Configure finished. Run build.bat next.
popd
endlocal
exit /b 0

:Failed
echo.
echo Configure FAILED.
popd
endlocal
exit /b 1

rem ---------------------------------------------------------------------------
rem  Locates cmake.exe. PATH first, then the usual install locations, including
rem  the copy Visual Studio ships so a machine without standalone CMake works.
rem ---------------------------------------------------------------------------
:FindCMake
set "CMAKE="
for /f "delims=" %%I in ('where cmake 2^>nul') do if not defined CMAKE set "CMAKE=%%I"
if defined CMAKE exit /b 0

for %%P in (
    "%ProgramFiles%\CMake\bin\cmake.exe"
    "%ProgramFiles(x86)%\CMake\bin\cmake.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
) do if not defined CMAKE if exist %%P set "CMAKE=%%~P"

if defined CMAKE exit /b 0

echo ERROR: cmake.exe was not found on PATH or in the usual install locations.
echo        Install CMake, or add it to PATH, and run this again.
exit /b 1
