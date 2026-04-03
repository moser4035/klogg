@echo off
setlocal EnableExtensions

for %%I in ("%~dp0..\..") do set "SCRIPT_DIR=%%~fI"

if not defined KLOGG_WORKSPACE set "KLOGG_WORKSPACE=%SCRIPT_DIR%"
if not defined KLOGG_BUILD_ROOT set "KLOGG_BUILD_ROOT=build_release"
if not defined KLOGG_BUILD_CONFIG set "KLOGG_BUILD_CONFIG=RelWithDebInfo"
if not defined KLOGG_BUILD_TARGET set "KLOGG_BUILD_TARGET=ci_build"
if not defined KLOGG_QT set "KLOGG_QT=Qt6"
if not defined KLOGG_ARCH set "KLOGG_ARCH=x64"
if not defined platform (
    if /I "%KLOGG_ARCH%"=="x86" (
        set "platform=Win32"
    ) else (
        set "platform=%KLOGG_ARCH%"
    )
)
if not defined KLOGG_CMAKE_GENERATOR call :detect_default_generator
if not defined KLOGG_CMAKE_SOURCE_CACHE set "KLOGG_CMAKE_SOURCE_CACHE=%KLOGG_WORKSPACE%\cpm_cache"
if not defined KLOGG_DO_INSTALLER set "KLOGG_DO_INSTALLER=1"
if not defined KLOGG_DO_PACKAGE set "KLOGG_DO_PACKAGE=1"
if not defined KLOGG_CMAKE_OPTS set "KLOGG_CMAKE_OPTS=-DKLOGG_GENERIC_CPU=ON -DKLOGG_USE_SENTRY=ON -DKLOGG_OVERRIDE_MALLOC=OFF"

set "BUILD_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%"

if not defined BOOST_ROOT (
    if exist "%KLOGG_WORKSPACE%\3rdparty\boost" (
        set "BOOST_ROOT=%KLOGG_WORKSPACE%\3rdparty\boost"
    )
)

call :load_existing_cache_settings
if errorlevel 1 exit /b 1

call :resolve_qt_cmake_dir
if errorlevel 1 exit /b 1

if defined KLOGG_GIT_SSL_BACKEND (
    echo Using GIT_SSL_BACKEND=%KLOGG_GIT_SSL_BACKEND% for this run.
    set "GIT_SSL_BACKEND=%KLOGG_GIT_SSL_BACKEND%"
)

echo Configuring CMake in "%BUILD_DIR%"...
call :configure_cmake
if errorlevel 1 exit /b 1

echo Running full release preparation...
set "KLOGG_DO_BUILD=1"
call "%~dp0prepare_release.cmd"
exit /b %errorlevel%

:configure_cmake
if exist "%CACHE_FILE%" (
    if defined KLOGG_QT_CMAKE_DIR (
        cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" -D%KLOGG_QT%_DIR="%KLOGG_QT_CMAKE_DIR%" %KLOGG_CMAKE_OPTS%
    ) else (
        cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" %KLOGG_CMAKE_OPTS%
    )
    if errorlevel 1 exit /b 1
    exit /b 0
)

if /I "%KLOGG_CMAKE_GENERATOR:~0,13%"=="Visual Studio" (
    if defined KLOGG_QT_CMAKE_DIR (
        cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -G "%KLOGG_CMAKE_GENERATOR%" -A "%platform%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" -D%KLOGG_QT%_DIR="%KLOGG_QT_CMAKE_DIR%" %KLOGG_CMAKE_OPTS%
    ) else (
        cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -G "%KLOGG_CMAKE_GENERATOR%" -A "%platform%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" %KLOGG_CMAKE_OPTS%
    )
    if errorlevel 1 exit /b 1
    exit /b 0
)

if defined KLOGG_QT_CMAKE_DIR (
    cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -G "%KLOGG_CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" -D%KLOGG_QT%_DIR="%KLOGG_QT_CMAKE_DIR%" %KLOGG_CMAKE_OPTS%
) else (
    cmake -S "%KLOGG_WORKSPACE%" -B "%BUILD_DIR%" -G "%KLOGG_CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%KLOGG_BUILD_CONFIG% -DCPM_SOURCE_CACHE="%KLOGG_CMAKE_SOURCE_CACHE%" %KLOGG_CMAKE_OPTS%
)
if errorlevel 1 exit /b 1
exit /b 0

:detect_default_generator
set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE_EXE%" (
    set "VS_VERSION="
    for /f "delims=" %%I in ('"%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion') do (
        set "VS_VERSION=%%I"
        goto :vs_version_found
    )
)
goto :generator_fallback

:vs_version_found
if /I "%VS_VERSION:~0,2%"=="18" (
    set "KLOGG_CMAKE_GENERATOR=Visual Studio 18 2026"
    exit /b 0
)
if /I "%VS_VERSION:~0,2%"=="17" (
    set "KLOGG_CMAKE_GENERATOR=Visual Studio 17 2022"
    exit /b 0
)

:generator_fallback
set "KLOGG_CMAKE_GENERATOR=Ninja"
exit /b 0

:load_existing_cache_settings
set "CACHE_FILE=%BUILD_DIR%\CMakeCache.txt"
if not exist "%CACHE_FILE%" exit /b 0

set "CACHED_CMAKE_GENERATOR="
set "CACHED_CMAKE_GENERATOR_INSTANCE="
set "CACHED_BUILD_TYPE="
set "CACHED_QT6_DIR="
set "CACHED_QT5_DIR="
set "CACHED_QT_DIR="

for /f "tokens=1,* delims==" %%A in ('findstr /R /B /C:"CMAKE_GENERATOR:INTERNAL=" /C:"CMAKE_GENERATOR_INSTANCE:INTERNAL=" /C:"CMAKE_BUILD_TYPE:STRING=" /C:"Qt6_DIR:" /C:"Qt5_DIR:" /C:"QT_DIR:" "%CACHE_FILE%"') do (
    if /I "%%A"=="CMAKE_GENERATOR:INTERNAL" set "CACHED_CMAKE_GENERATOR=%%B"
    if /I "%%A"=="CMAKE_GENERATOR_INSTANCE:INTERNAL" set "CACHED_CMAKE_GENERATOR_INSTANCE=%%B"
    if /I "%%A"=="CMAKE_BUILD_TYPE:STRING" set "CACHED_BUILD_TYPE=%%B"
    if /I "%%A"=="Qt6_DIR:PATH" set "CACHED_QT6_DIR=%%B"
    if /I "%%A"=="Qt6_DIR:UNINITIALIZED" set "CACHED_QT6_DIR=%%B"
    if /I "%%A"=="Qt5_DIR:PATH" set "CACHED_QT5_DIR=%%B"
    if /I "%%A"=="Qt5_DIR:UNINITIALIZED" set "CACHED_QT5_DIR=%%B"
    if /I "%%A"=="QT_DIR:PATH" set "CACHED_QT_DIR=%%B"
    if /I "%%A"=="QT_DIR:UNINITIALIZED" set "CACHED_QT_DIR=%%B"
)

if defined CACHED_CMAKE_GENERATOR (
    if /I "%KLOGG_CMAKE_GENERATOR%"=="Ninja" (
        echo Reusing existing CMake generator "%CACHED_CMAKE_GENERATOR%" from "%CACHE_FILE%".
        set "KLOGG_CMAKE_GENERATOR=%CACHED_CMAKE_GENERATOR%"
        if defined CACHED_CMAKE_GENERATOR_INSTANCE set "KLOGG_CMAKE_GENERATOR_INSTANCE=%CACHED_CMAKE_GENERATOR_INSTANCE%"
    ) else (
        if /I not "%KLOGG_CMAKE_GENERATOR%"=="%CACHED_CMAKE_GENERATOR%" (
            echo Error: Existing build directory "%BUILD_DIR%" uses generator "%CACHED_CMAKE_GENERATOR%".
            echo Set KLOGG_BUILD_ROOT to a different directory or remove the existing CMake cache.
            exit /b 1
        )
    )
)

if not defined KLOGG_QT_DIR (
    if /I "%KLOGG_QT%"=="Qt6" (
        if defined CACHED_QT6_DIR (
            set "KLOGG_QT_DIR=%CACHED_QT6_DIR%"
            set "KLOGG_QT_CMAKE_DIR=%CACHED_QT6_DIR%"
        ) else (
            if defined CACHED_QT_DIR (
                set "KLOGG_QT_DIR=%CACHED_QT_DIR%"
                set "KLOGG_QT_CMAKE_DIR=%CACHED_QT_DIR%"
            )
        )
    ) else (
        if defined CACHED_QT5_DIR (
            set "KLOGG_QT_DIR=%CACHED_QT5_DIR%"
            set "KLOGG_QT_CMAKE_DIR=%CACHED_QT5_DIR%"
        ) else (
            if defined CACHED_QT_DIR (
                set "KLOGG_QT_DIR=%CACHED_QT_DIR%"
                set "KLOGG_QT_CMAKE_DIR=%CACHED_QT_DIR%"
            )
        )
    )
)

if defined CACHED_BUILD_TYPE (
    if /I "%KLOGG_BUILD_CONFIG%"=="Release" (
        set "KLOGG_BUILD_CONFIG=%CACHED_BUILD_TYPE%"
    )
)

exit /b 0

:resolve_qt_cmake_dir
if defined KLOGG_QT_DIR (
    call :normalize_qt_dir "%KLOGG_QT_DIR%"
    exit /b %errorlevel%
)

if /I "%KLOGG_QT%"=="Qt6" (
    if defined Qt6_DIR (
        set "KLOGG_QT_CMAKE_DIR=%Qt6_DIR%"
        set "KLOGG_QT_DIR=%Qt6_DIR%"
        exit /b 0
    )
) else (
    if defined Qt5_DIR (
        set "KLOGG_QT_CMAKE_DIR=%Qt5_DIR%"
        set "KLOGG_QT_DIR=%Qt5_DIR%"
        exit /b 0
    )
)

echo Warning: No Qt CMake directory configured. Relying on CMake package discovery.
exit /b 0

:normalize_qt_dir
set "QT_INPUT=%~1"
set "KLOGG_QT_CMAKE_DIR=%QT_INPUT%"

if exist "%QT_INPUT%\bin\windeployqt.exe" (
    set "KLOGG_QT_DIR=%QT_INPUT%"
    set "KLOGG_QT_CMAKE_DIR=%QT_INPUT%\lib\cmake\%KLOGG_QT%"
    exit /b 0
)

if exist "%QT_INPUT%\bin" (
    set "KLOGG_QT_DIR=%QT_INPUT%"
    set "KLOGG_QT_CMAKE_DIR=%QT_INPUT%\lib\cmake\%KLOGG_QT%"
    exit /b 0
)

if exist "%QT_INPUT%\..\..\..\bin\windeployqt.exe" (
    for %%I in ("%QT_INPUT%\..\..\..") do set "KLOGG_QT_DIR=%%~fI"
    exit /b 0
)

echo Warning: Could not infer Qt runtime root from "%QT_INPUT%".
set "KLOGG_QT_DIR=%QT_INPUT%"
exit /b 0
