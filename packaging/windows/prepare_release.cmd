@echo off
setlocal EnableExtensions EnableDelayedExpansion

for %%I in ("%~dp0..\..") do set "SCRIPT_DIR=%%~fI"
set "COPY_IF_EXISTS=%~dp0copy_if_exists.cmd"

if not defined KLOGG_WORKSPACE set "KLOGG_WORKSPACE=%SCRIPT_DIR%"
if not defined KLOGG_BUILD_ROOT set "KLOGG_BUILD_ROOT=build_release"
if not defined KLOGG_BUILD_CONFIG set "KLOGG_BUILD_CONFIG=Release"
if not defined KLOGG_QT set "KLOGG_QT=Qt6"
if not defined KLOGG_ARCH set "KLOGG_ARCH=x64"
if not defined platform set "platform=%KLOGG_ARCH%"
if not defined KLOGG_VERSION set "KLOGG_VERSION=dev-build"
if not defined KLOGG_DO_BUILD set "KLOGG_DO_BUILD=0"
if not defined KLOGG_BUILD_TARGET set "KLOGG_BUILD_TARGET=ci_build"
if not defined KLOGG_DO_INSTALLER set "KLOGG_DO_INSTALLER=0"
if not defined KLOGG_DO_PACKAGE set "KLOGG_DO_PACKAGE=0"
if not defined KLOGG_BUILD_PARALLEL set "KLOGG_BUILD_PARALLEL="

set "BUILD_OUTPUT_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\%KLOGG_BUILD_CONFIG%"
if not exist "%BUILD_OUTPUT_DIR%\klogg.exe" set "BUILD_OUTPUT_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output"
set "GENERATED_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\generated"
set "BUILD_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%"
set "RELEASE_DIR=%BUILD_DIR%\release"
set "CHOCO_DIR=%BUILD_DIR%\chocolatey"
set "PACKAGES_DIR=%BUILD_DIR%\packages"
set "PORTABLE_ZIP=%PACKAGES_DIR%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip"
set "PDB_ZIP=%PACKAGES_DIR%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip"
set "SETUP_EXE=%PACKAGES_DIR%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-setup.exe"

if not exist "%PACKAGES_DIR%" mkdir "%PACKAGES_DIR%"

if /I "%KLOGG_DO_BUILD%"=="1" (
    call :build_release
    if errorlevel 1 exit /b 1
)

if not exist "%BUILD_OUTPUT_DIR%\klogg.exe" (
    echo Could not find klogg.exe under "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%".
    echo Expected either "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output\%KLOGG_BUILD_CONFIG%" or "%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\output".
    exit /b 1
)

call :detect_qt_dir
if not exist "%BUILD_OUTPUT_DIR%\%KLOGG_QT%Core.dll" (
    if defined KLOGG_QT_DIR (
        if exist "%KLOGG_QT_DIR%\bin\windeployqt.exe" (
            echo Running windeployqt from "%KLOGG_QT_DIR%\bin\windeployqt.exe"...
            set "PATH=%KLOGG_QT_DIR%\bin;%PATH%"
            "%KLOGG_QT_DIR%\bin\windeployqt.exe" "%BUILD_OUTPUT_DIR%\klogg.exe"
        ) else (
            echo Warning: windeployqt.exe not found under "%KLOGG_QT_DIR%\bin".
        )
    ) else (
        echo Warning: KLOGG_QT_DIR is not set and could not be inferred from CMakeCache.txt.
        echo Warning: Qt runtime files will only be packaged if they already exist in "%BUILD_OUTPUT_DIR%".
    )
)

if defined KLOGG_QT_DIR (
    call "%COPY_IF_EXISTS%" "%KLOGG_QT_DIR%\bin\%KLOGG_QT%Concurrent.dll" "%BUILD_OUTPUT_DIR%\%KLOGG_QT%Concurrent.dll"
)

if exist "%RELEASE_DIR%" rmdir /S /Q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

echo Copying build output from "%BUILD_OUTPUT_DIR%"...
xcopy "%BUILD_OUTPUT_DIR%\*" "%RELEASE_DIR%\" /E /I /Y >nul

echo Copying documentation...
call "%COPY_IF_EXISTS%" "%GENERATED_DIR%\documentation.html" "%RELEASE_DIR%\documentation.html"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\COPYING" "%RELEASE_DIR%\COPYING"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\NOTICE" "%RELEASE_DIR%\NOTICE"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\README.md" "%RELEASE_DIR%\README.md"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\DOCUMENTATION.md" "%RELEASE_DIR%\DOCUMENTATION.md"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\packaging\windows\openssl-1.1\LICENSE" "%RELEASE_DIR%\OpenSSL-LICENSE.txt"

echo Copying VC runtime...
call :detect_vc_redist
if defined VC_REDIST_DIR (
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\msvcp140.dll" "%RELEASE_DIR%\msvcp140.dll"
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\msvcp140_1.dll" "%RELEASE_DIR%\msvcp140_1.dll"
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\msvcp140_2.dll" "%RELEASE_DIR%\msvcp140_2.dll"
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\vcruntime140.dll" "%RELEASE_DIR%\vcruntime140.dll"
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\vcruntime140_1.dll" "%RELEASE_DIR%\vcruntime140_1.dll"
    call "%COPY_IF_EXISTS%" "%VC_REDIST_DIR%\concrt140.dll" "%RELEASE_DIR%\concrt140.dll"
) else (
    echo Warning: Could not locate the MSVC redistributable directory.
)

call :detect_ssl_dir
if defined SSL_DIR (
    echo Copying OpenSSL from "%SSL_DIR%"...
    if /I "%KLOGG_ARCH%"=="x64" (
        call "%COPY_IF_EXISTS%" "%SSL_DIR%\libcrypto-1_1-x64.dll" "%RELEASE_DIR%\libcrypto-1_1-x64.dll"
        call "%COPY_IF_EXISTS%" "%SSL_DIR%\libssl-1_1-x64.dll" "%RELEASE_DIR%\libssl-1_1-x64.dll"
    ) else (
        call "%COPY_IF_EXISTS%" "%SSL_DIR%\libcrypto-1_1.dll" "%RELEASE_DIR%\libcrypto-1_1.dll"
        call "%COPY_IF_EXISTS%" "%SSL_DIR%\libssl-1_1.dll" "%RELEASE_DIR%\libssl-1_1.dll"
    )
) else (
    echo SSL_DIR is not set. Skipping OpenSSL runtime copy.
)

echo Copying packaging files...
if exist "%CHOCO_DIR%" rmdir /S /Q "%CHOCO_DIR%"
mkdir "%CHOCO_DIR%"
if not exist "%CHOCO_DIR%\tools" mkdir "%CHOCO_DIR%\tools"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\packaging\windows\chocolatey\klogg.nuspec" "%CHOCO_DIR%\klogg.nuspec"
call "%COPY_IF_EXISTS%" "%KLOGG_WORKSPACE%\packaging\windows\chocolatey\tools\chocolateyInstall.ps1" "%CHOCO_DIR%\tools\chocolateyInstall.ps1"

call :detect_7z
if "%SEVENZIP_EXE%"=="" (
    echo 7z.exe not found. Skipping portable archives.
    goto :after_archives
)

echo Using 7-Zip from "%SEVENZIP_EXE%".
echo Making portable archive...
if exist "%PORTABLE_ZIP%" del "%PORTABLE_ZIP%"

"%SEVENZIP_EXE%" a -r "%PORTABLE_ZIP%" "%RELEASE_DIR%\*.dll" >nul
if exist "%RELEASE_DIR%\klogg.exe" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\klogg.exe" >nul
if exist "%RELEASE_DIR%\klogg_portable.exe" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\klogg_portable.exe" >nul
if exist "%RELEASE_DIR%\klogg_crashpad_handler.exe" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\klogg_crashpad_handler.exe" >nul
if exist "%RELEASE_DIR%\klogg_minidump_dump.exe" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\klogg_minidump_dump.exe" >nul
if exist "%RELEASE_DIR%\documentation.html" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\documentation.html" >nul
if exist "%RELEASE_DIR%\COPYING" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\COPYING" >nul
if exist "%RELEASE_DIR%\NOTICE" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\NOTICE" >nul
if exist "%RELEASE_DIR%\README.md" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\README.md" >nul
if exist "%RELEASE_DIR%\DOCUMENTATION.md" "%SEVENZIP_EXE%" a "%PORTABLE_ZIP%" "%RELEASE_DIR%\DOCUMENTATION.md" >nul

echo Making PDB archive...
if exist "%PDB_ZIP%" del "%PDB_ZIP%"

if exist "%RELEASE_DIR%\*.pdb" "%SEVENZIP_EXE%" a "%PDB_ZIP%" "%RELEASE_DIR%\*.pdb" >nul

if not exist "%PDB_ZIP%" echo No PDB files found. Skipping PDB archive.

:after_archives
if /I "%KLOGG_DO_INSTALLER%"=="1" (
    if not defined MAKENSIS_EXE (
        if defined KLOGG_MAKENSIS_EXE (
            if exist "%KLOGG_MAKENSIS_EXE%" set "MAKENSIS_EXE=%KLOGG_MAKENSIS_EXE%"
        )
    )
    if not defined MAKENSIS_EXE (
        if exist "%ProgramFiles%\NSIS\makensis.exe" set "MAKENSIS_EXE=%ProgramFiles%\NSIS\makensis.exe"
    )
    if not defined MAKENSIS_EXE (
        if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "MAKENSIS_EXE=%ProgramFiles(x86)%\NSIS\makensis.exe"
    )
    if not defined MAKENSIS_EXE (
        for /f "delims=" %%I in ('where makensis.exe 2^>nul') do (
            set "MAKENSIS_EXE=%%I"
        )
    )
    if not defined MAKENSIS_EXE (
        echo makensis.exe not found. Skipping installer build.
    ) else (
        echo Using NSIS from "!MAKENSIS_EXE!".
        echo Building NSIS installer...
        "!MAKENSIS_EXE!" /DVERSION=%KLOGG_VERSION% /DPLATFORM=%KLOGG_ARCH% /DQT_MAJOR=%KLOGG_QT% /DSTAGE_DIR=%RELEASE_DIR% /DOUTPUT_DIR=%PACKAGES_DIR% "%KLOGG_WORKSPACE%\packaging\windows\klogg.nsi"
        if errorlevel 1 exit /b 1
    )
)

if /I "%KLOGG_DO_PACKAGE%"=="1" (
    echo Collecting packages into "%PACKAGES_DIR%"...
    if not exist "%PORTABLE_ZIP%" echo Warning: Missing "%PORTABLE_ZIP%"
    if not exist "%PDB_ZIP%" echo Warning: Missing "%PDB_ZIP%"
    if not exist "%SETUP_EXE%" echo Warning: Missing "%SETUP_EXE%"
)

:done
echo Done!
goto :eof

:build_release
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Could not find "%BUILD_DIR%\CMakeCache.txt".
    echo Configure the build directory first, then rerun this script.
    exit /b 1
)

call :detect_cmake_generator

echo Building "%KLOGG_BUILD_TARGET%" in "%BUILD_DIR%"...
set "BUILD_PARALLEL_ARG="
if defined KLOGG_BUILD_PARALLEL (
    set "BUILD_PARALLEL_ARG=--parallel %KLOGG_BUILD_PARALLEL%"
) else (
    if /I "%CMAKE_GENERATOR_NAME:~0,13%"=="Visual Studio" (
        set "BUILD_PARALLEL_ARG=--parallel 1"
    )
)

cmake --build "%BUILD_DIR%" --config "%KLOGG_BUILD_CONFIG%" --target "%KLOGG_BUILD_TARGET%" %BUILD_PARALLEL_ARG%
if errorlevel 1 exit /b 1
goto :eof

:detect_qt_dir
if defined KLOGG_QT_DIR (
    call :normalize_qt_dir "%KLOGG_QT_DIR%"
    goto :eof
)
set "CACHE_FILE=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\CMakeCache.txt"
if not exist "%CACHE_FILE%" goto :eof

set "QT_CONFIG_DIR="
set "QT_PREFIX_DIR="
for /f "tokens=1,* delims==" %%A in ('findstr /R /B /C:"Qt6_DIR:" /C:"Qt5_DIR:" /C:"QT_DIR:" /C:"CMAKE_PREFIX_PATH:" "%CACHE_FILE%"') do (
    if /I "%%A"=="Qt6_DIR:PATH" set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="Qt6_DIR:UNINITIALIZED" set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="Qt5_DIR:PATH" set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="Qt5_DIR:UNINITIALIZED" set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="QT_DIR:PATH" if not defined QT_CONFIG_DIR set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="QT_DIR:UNINITIALIZED" if not defined QT_CONFIG_DIR set "QT_CONFIG_DIR=%%B"
    if /I "%%A"=="CMAKE_PREFIX_PATH:PATH" if not defined QT_PREFIX_DIR set "QT_PREFIX_DIR=%%B"
    if /I "%%A"=="CMAKE_PREFIX_PATH:UNINITIALIZED" if not defined QT_PREFIX_DIR set "QT_PREFIX_DIR=%%B"
)
if defined QT_CONFIG_DIR (
    call :normalize_qt_dir "%QT_CONFIG_DIR%"
    goto :eof
)
if defined QT_PREFIX_DIR call :normalize_qt_dir "%QT_PREFIX_DIR%"
goto :eof

:normalize_qt_dir
set "QT_INPUT=%~1"
set "KLOGG_QT_DIR=%QT_INPUT%"

if exist "%QT_INPUT%\bin\windeployqt.exe" goto :eof
if exist "%QT_INPUT%\bin" goto :eof

set "KLOGG_QT_DIR=%QT_INPUT:\lib\cmake\Qt6=%"
set "KLOGG_QT_DIR=%KLOGG_QT_DIR:\lib\cmake\Qt5=%"
if exist "%KLOGG_QT_DIR%\bin\windeployqt.exe" goto :eof
if exist "%QT_INPUT%\..\..\..\bin\windeployqt.exe" (
    for %%I in ("%QT_INPUT%\..\..\..") do set "KLOGG_QT_DIR=%%~fI"
)
goto :eof

:detect_cmake_generator
if defined CMAKE_GENERATOR_NAME goto :eof
set "CACHE_FILE=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%\CMakeCache.txt"
if not exist "%CACHE_FILE%" goto :eof

for /f "tokens=1,* delims==" %%A in ('findstr /B /C:"CMAKE_GENERATOR:INTERNAL=" "%CACHE_FILE%"') do set "CMAKE_GENERATOR_NAME=%%B"
goto :eof

:detect_vc_redist
if defined VC_REDIST_DIR goto :eof

if defined VCToolsRedistDir (
    set "VC_REDIST_DIR=%VCToolsRedistDir%%platform%\Microsoft.VC143.CRT"
    if exist "%VC_REDIST_DIR%\vcruntime140.dll" goto :eof
    set "VC_REDIST_DIR="
)

set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE_EXE%" goto :eof

set "VC_REDIST_FILE="
for /f "delims=" %%I in ('"%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Redist\MSVC\**\%platform%\Microsoft.VC143.CRT\vcruntime140.dll') do (
    set "VC_REDIST_FILE=%%I"
    goto :vc_redist_found
)
goto :eof

:vc_redist_found
for %%I in ("%VC_REDIST_FILE%") do set "VC_REDIST_DIR=%%~dpI"
if "%VC_REDIST_DIR:~-1%"=="\" set "VC_REDIST_DIR=%VC_REDIST_DIR:~0,-1%"
goto :eof

:detect_ssl_dir
if defined SSL_DIR goto :eof

if /I "%KLOGG_ARCH%"=="x64" (
    set "SSL_ARCH_DIR=x64"
    set "SSL_EXPECTED_DLL=libcrypto-1_1-x64.dll"
) else (
    set "SSL_ARCH_DIR=x86"
    set "SSL_EXPECTED_DLL=libcrypto-1_1.dll"
)

if exist "%KLOGG_WORKSPACE%\packaging\windows\openssl-1.1\%SSL_ARCH_DIR%\bin\%SSL_EXPECTED_DLL%" (
    set "SSL_DIR=%KLOGG_WORKSPACE%\packaging\windows\openssl-1.1\%SSL_ARCH_DIR%\bin"
)
goto :eof

:detect_7z
if defined SEVENZIP_EXE goto :eof
if defined KLOGG_7Z_EXE (
    if exist "%KLOGG_7Z_EXE%" (
        set "SEVENZIP_EXE=%KLOGG_7Z_EXE%"
        goto :eof
    )
)
if exist "%ProgramFiles%\7-Zip\7z.exe" (
    set "SEVENZIP_EXE=%ProgramFiles%\7-Zip\7z.exe"
    goto :eof
)
if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" (
    set "SEVENZIP_EXE=%ProgramFiles(x86)%\7-Zip\7z.exe"
    goto :eof
)
for /f "delims=" %%I in ('where 7z.exe 2^>nul') do (
    set "SEVENZIP_EXE=%%I"
    goto :eof
)
goto :eof
