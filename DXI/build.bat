@echo off
rem ---------------------------------------------------------------------------
rem  build.bat | Builds DXIApp
rem
rem  Usage:
rem      build.bat                      Debug
rem      build.bat Release              bare word is taken as the configuration
rem      build.bat --config Release     same thing, spelled out
rem      build.bat -c Release
rem      build.bat --target DXIApp      a different CMake target
rem      build.bat --clean              rebuild from scratch
rem      build.bat --help
rem
rem  Configures first when there is no build tree yet, so a fresh clone only
rem  needs this one script.
rem
rem  Retries the build once on failure. That is not papering over errors: the
rem  CONFIGURE_DEPENDS globs in CMakeLists.txt re-run at build time, after
rem  MSBuild has already loaded the project, so the first build after a file is
rem  added or deleted cannot see it and fails at link. By the second pass the
rem  re-glob has happened and the file is in the project. A genuine compile
rem  error fails both passes and the script exits non zero.
rem ---------------------------------------------------------------------------

setlocal
pushd "%~dp0"

set "CONFIG=Debug"
set "TARGET=DXIApp"
set "CLEAN="

:ParseArgs
if "%~1"=="" goto :ArgsDone

if /i "%~1"=="--help"   goto :Usage
if /i "%~1"=="-h"       goto :Usage
if /i "%~1"=="/?"       goto :Usage
if /i "%~1"=="--config" goto :SetConfig
if /i "%~1"=="-c"       goto :SetConfig
if /i "%~1"=="--target" goto :SetTarget
if /i "%~1"=="-t"       goto :SetTarget
if /i "%~1"=="--clean"  goto :SetClean

rem Anything else is taken as the configuration, so "build.bat Release" works
set "CONFIG=%~1"
shift
goto :ParseArgs

:SetConfig
if "%~2"=="" (
    echo ERROR: --config needs a value, for example --config Release
    goto :Usage
)
set "CONFIG=%~2"
shift
shift
goto :ParseArgs

:SetTarget
if "%~2"=="" (
    echo ERROR: --target needs a value, for example --target DXIApp
    goto :Usage
)
set "TARGET=%~2"
shift
shift
goto :ParseArgs

:SetClean
set "CLEAN=--clean-first"
shift
goto :ParseArgs

:ArgsDone

rem The configurations the Visual Studio generator produces. Catching a typo here
rem is far clearer than letting MSBuild fail on an unknown configuration later.
if /i "%CONFIG%"=="Debug"          goto :ConfigOk
if /i "%CONFIG%"=="Release"        goto :ConfigOk
if /i "%CONFIG%"=="RelWithDebInfo" goto :ConfigOk
if /i "%CONFIG%"=="MinSizeRel"     goto :ConfigOk

echo ERROR: unknown configuration "%CONFIG%".
echo        Expected one of Debug, Release, RelWithDebInfo, MinSizeRel.
goto :Usage

:ConfigOk

call :FindCMake
if errorlevel 1 goto :Failed

if not exist "build\CMakeCache.txt" (
    echo No build tree yet, configuring first.
    echo.
    "%CMAKE%" -S . -B build
    if errorlevel 1 goto :Failed
    echo.
)

echo Building %TARGET% ^| %CONFIG%
echo.
"%CMAKE%" --build build --config %CONFIG% --target %TARGET% %CLEAN%
if not errorlevel 1 goto :Succeeded

echo.
echo ---------------------------------------------------------------------------
echo  First pass failed. Retrying once, in case a file was added or removed and
echo  only reached the project on the re-glob that just ran.
echo ---------------------------------------------------------------------------
echo.
"%CMAKE%" --build build --config %CONFIG% --target %TARGET% %CLEAN%
if errorlevel 1 goto :Failed

:Succeeded
echo.
echo Build finished: build\bin\%CONFIG%\DXIApp.exe
popd
endlocal
exit /b 0

:Usage
echo.
echo Usage: build.bat [configuration] [options]
echo.
echo   configuration        Debug (default), Release, RelWithDebInfo, MinSizeRel
echo.
echo   -c, --config NAME    Configuration, same as the bare word above
echo   -t, --target NAME    CMake target to build (default DXIApp)
echo       --clean          Rebuild from scratch
echo   -h, --help           This text
echo.
popd
endlocal
exit /b 2

:Failed
echo.
echo Build FAILED.
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
