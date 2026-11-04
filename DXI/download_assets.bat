@echo off
rem ---------------------------------------------------------------------------
rem  download_assets.bat | Fetches the models the repository does not carry
rem
rem  Meshes are downloaded rather than committed. Every Visual Studio .gitignore
rem  carries a *.obj rule for MSVC object files, which silently swallows Wavefront
rem  .obj meshes as well, so committing them needs a fighting exception for a file
rem  that is better fetched anyway.
rem
rem  Usage:
rem      download_assets.bat            fetch anything missing
rem      download_assets.bat --force    fetch again even if present
rem
rem  Needs curl.exe and tar.exe, both shipped in System32 on Windows 10 1803 and
rem  later. setup.bat calls this, so the normal flow needs nothing extra.
rem ---------------------------------------------------------------------------

setlocal
pushd "%~dp0"

set "MODELROOT=DXI\models"
set "FORCE="

if /i "%~1"=="--force" set "FORCE=1"
if /i "%~1"=="-f"      set "FORCE=1"
if /i "%~1"=="--help"  goto :Usage
if /i "%~1"=="-h"      goto :Usage

call :Need curl.exe
if errorlevel 1 goto :Failed
call :Need tar.exe
if errorlevel 1 goto :Failed

rem  Name, archive, and which files to lift out of it.
rem  Spot is by Keenan Crane, dedicated to the public domain under CC0 1.0.
rem  https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/
call :Fetch "spot" ^
    "https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/spot.zip" ^
    "spot_triangulated.obj spot_texture.png README.txt"
if errorlevel 1 goto :Failed

echo.
echo Assets are up to date.
popd
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :Fetch  name  url  "file1 file2 ..."
rem  Downloads, extracts, and lifts the wanted files out of wherever they sit in
rem  the archive. A README is renamed so the licence is obvious in the folder.
rem ---------------------------------------------------------------------------
:Fetch
set "NAME=%~1"
set "URL=%~2"
set "WANTED=%~3"
set "DEST=%MODELROOT%\%NAME%"

if not defined FORCE if exist "%DEST%\*.obj" (
    echo [skip] %NAME% is already here
    exit /b 0
)

echo [get ] %NAME%

set "WORK=%TEMP%\dxi_asset_%NAME%"
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%" 2>nul

curl.exe -L --fail --silent --show-error -o "%WORK%\archive.zip" "%URL%"
if errorlevel 1 (
    echo        download failed: %URL%
    exit /b 1
)

tar.exe -xf "%WORK%\archive.zip" -C "%WORK%"
if errorlevel 1 (
    echo        could not extract the archive
    exit /b 1
)

if not exist "%DEST%" mkdir "%DEST%"

for %%F in (%WANTED%) do call :CopyOut "%WORK%" "%%F" "%DEST%"

rem Make the licence obvious rather than leaving it as a generic readme
if exist "%DEST%\README.txt" (
    if exist "%DEST%\LICENSE_AND_README.txt" del /q "%DEST%\LICENSE_AND_README.txt"
    ren "%DEST%\README.txt" "LICENSE_AND_README.txt"
)

rmdir /s /q "%WORK%"
echo        done, see %DEST%
exit /b 0

rem ---------------------------------------------------------------------------
rem  :CopyOut  searchRoot  fileName  destination
rem  Archives nest their contents differently, so the file is hunted for rather
rem  than assumed to sit at a fixed depth.
rem ---------------------------------------------------------------------------
:CopyOut
set "FOUND="
for /r "%~1" %%G in ("%~2") do if not defined FOUND if exist "%%G" set "FOUND=%%G"

if not defined FOUND (
    echo        WARNING: %~2 was not in the archive
    exit /b 0
)

copy /y "%FOUND%" "%~3\" >nul
exit /b 0

rem ---------------------------------------------------------------------------
:Need
where %~1 >nul 2>nul
if errorlevel 1 (
    echo ERROR: %~1 was not found on PATH.
    echo        It ships in System32 on Windows 10 1803 and later.
    exit /b 1
)
exit /b 0

:Usage
echo.
echo Usage: download_assets.bat [--force]
echo.
echo   --force, -f   Download again even when the files are already present
echo   --help,  -h   This text
echo.
popd
endlocal
exit /b 2

:Failed
echo.
echo Asset download FAILED.
popd
endlocal
exit /b 1
