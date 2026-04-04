# How to Build Klogg

## Overview

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.
Local builds can be faster because code can be optimized for current CPU instead of generic x86-64. Support for SSE4/AVX code paths
will be enabled if available on build machine.

## Getting the Source

This project is [hosted on GitHub](https://github.com/moser4035/klogg). You can clone this project directly using this command:

```
git clone https://github.com/moser4035/klogg
```

## Dependencies

To build Klogg:

- cmake 3.12 or later to generate build files
- C++ compiler with decent C++17 support (at least gcc 7.5, clang 7, msvc 19.14)
- Qt libraries 5.9 or later (CI builds use Qt 5.9.5/5.12.5/5.15.2):
  - QtCore
  - QtGui
  - QtWidgets
  - QtConcurrent
  - QtNetwork
  - QtXml
  - QtTools

To build Hyperscan regular expressions backend (default):

- CPU with support for [SSSE3](https://en.wikipedia.org/wiki/SSSE3) instructions (for Hyperscan backend)
- Boost (1.58 or later, header-only part)
- Ragel (6.8 or later; precompiled binary is provided for Windows; has to be installed from package managers on Linux or Homebrew on Mac)

To build installer for Windows:

- nsis to build installer for Windows
- Precompiled OpenSSl library to enable https support on Windows

Building tests:

- QtTest

All other dependencies are provided by [CPM](https://github.com/cpm-cmake/CPM.cmake) during cmake configuration stage (see 3rdparty directory).

CPM will try to find Hyperscan, TBB, uchardet and xxhash installed on build host.
If a library can't be found, the one provided by CPM will be used.

## Building

### Directory layout

The source tree should stay clean. Generated content is expected to live under the build directory you choose.

- Source tree:
  - `packaging/windows/` contains the maintained Windows helper scripts and installer definition
  - `packaging/windows/openssl-1.1/` can optionally hold the prebuilt OpenSSL runtime DLLs used for Windows packaging
- Development build output:
  - `build_debug/` (or another directory you choose) contains CMake files, object files and `output/`
- Release build output:
  - `build_release/output/<config>/` contains compiled binaries
  - `build_release/release/` contains the staged files used for packaging
  - `build_release/chocolatey/` contains local Chocolatey staging files
  - `build_release/packages/` contains the final portable ZIP, installer and optional PDB ZIP

### Configuration options

By default Klogg is built without support for reporting crash dumps. This can be enabled via cmake option `-DKLOGG_USE_SENTRY=ON`.

Klogg uses Hyperscan regular expressions library which requires CPU with SSSE3 support, ragel and boost headers.
Klogg can be built with only Qt reqular expressions backend by passing `-DKLOGG_USE_HYPERSCAN=OFF` to cmake.

Klogg can use custom memory allocator. By default it uses TBB memory allocator for Windows, mimalloc on Linux and default system allocator on MacOS.
Memory allocator override can be turned off by passing `-DKLOGG_OVERRIDE_MALLOC`. If you want to use TBB allocator on Linux then pass
`-DKLOGG_USE_MIMALLOC=OFF`.

### Building on Linux

Here is how to build klogg on Ubuntu 18.04.

Install dependencies:

```
sudo apt-get install build-essential cmake qtbase5-dev libboost-all-dev ragel
```

Configure and build klogg:

```
cd <path_to_klogg_repository_clone>
mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
```

**_If cmake gives error about missing "Qt5LinguistTools" configuration files, try running:_**

```bash
sudo apt-get install qttools5-dev
```

Binaries are placed into `build_debug/output`.

See `.github/workflows/ci-build.yml` for more information on build process.

### Building on Windows

The recommended local Windows target is `x64` with `Qt6` and a current Visual Studio toolchain.
The supported local workflows are:

- Development or debug build: configure and build directly with CMake
- Release build: run `packaging/windows/build_release_bundle.cmd`

#### Prerequisites

- Visual Studio 2022 or newer with Desktop C++ tools
- CMake 3.21 or newer
- Qt 6 for MSVC x64
- Boost headers, with `BOOST_ROOT` pointing to the extracted Boost directory
- Optional for release packaging:
  - 7-Zip for portable ZIP archives
  - NSIS 3 for the installer
  - prebuilt OpenSSL 1.1 runtime DLLs in `packaging/windows/openssl-1.1/x64/bin/`

The current packaging scripts will automatically pick up:

- `Qt6_DIR` or `KLOGG_QT_DIR`
- `BOOST_ROOT`
- `packaging/windows/openssl-1.1/x64/bin/libcrypto-1_1-x64.dll`
- `packaging/windows/openssl-1.1/x64/bin/libssl-1_1-x64.dll`

Use a Developer PowerShell or Developer Command Prompt so the MSVC toolchain is already available.

#### Development or debug build

This is the preferred flow when you are iterating locally and do not need release packaging.

Configure:

```powershell
cmake -S . -B build_debug `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -DQt6_DIR="C:\Qt\6.11.0\msvc2022_64\lib\cmake\Qt6" `
  -DBOOST_ROOT="C:\Boost\boost_1_82_0"
```

Build the main application:

```powershell
cmake --build build_debug --config RelWithDebInfo --target klogg
```

Useful locations:

- binaries: `build_debug/output/RelWithDebInfo/`
- generated headers and docs: `build_debug/generated/`

Run tests:

```powershell
ctest --test-dir build_debug --build-config RelWithDebInfo --output-on-failure
```

If you want a faster Debug-style iteration loop, replace `RelWithDebInfo` with `Debug`.

#### Release build

Use the wrapper script when you want a complete local Windows release run, including:

- CMake configure
- application build
- `windeployqt`
- staging files into a build-local `release/` directory
- portable ZIP generation
- optional NSIS installer
- optional PDB ZIP when building with `RelWithDebInfo`

The top-level entry point is:

```cmd
packaging\windows\build_release_bundle.cmd
```

Recommended environment variables before running it:

```cmd
set BOOST_ROOT=C:\Boost\boost_1_82_0
set KLOGG_QT_DIR=C:\Qt\6.11.0\msvc2022_64
set KLOGG_BUILD_ROOT=build_release
set KLOGG_BUILD_CONFIG=RelWithDebInfo
packaging\windows\build_release_bundle.cmd
```

Notes:

- `build_release_bundle.cmd` is the full local release entry point
- `prepare_release.cmd` packages an already-configured build directory and can optionally build it, but it does not perform the initial CMake configure step
- `RelWithDebInfo` is recommended for local releases because it keeps debug symbols and enables the `*-pdb.zip` artifact
- if you switch to plain `Release`, the main installer and portable ZIP are still produced, but there may be no PDB ZIP

Useful release output locations:

- compiled binaries: `build_release/output/RelWithDebInfo/`
- staged package contents: `build_release/release/`
- final deliverables: `build_release/packages/`
- Chocolatey staging: `build_release/chocolatey/`

The final package directory can contain:

- `klogg-<version>-x64-Qt6-portable.zip`
- `klogg-<version>-x64-Qt6-setup.exe`
- `klogg-<version>-x64-Qt6-pdb.zip`

If `makensis.exe` or `7z.exe` are installed in standard locations, the scripts will detect them automatically.
You can also override discovery explicitly:

```cmd
set KLOGG_MAKENSIS_EXE=C:\Program Files (x86)\NSIS\makensis.exe
set KLOGG_7Z_EXE=C:\Program Files\7-Zip\7z.exe
```

### Building on Mac OS

Klogg requires macOS High Sierra (10.13) or higher.

Install [Homebrew](https://brew.sh/) using terminal:

```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Homebrew installer should also install xcode command line tools.

Download and install build dependencies:

```
brew install cmake ninja qt boost ragel
```

Usually path to qt installation looks like `/usr/local/Cellar/qt/5.14.0/lib/cmake/Qt5`

Configure and build klogg:

```
cd <path_to_klogg_repository_clone>
mkdir build_debug
cd build_debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQt5_DIR=<path_to_qt_install> ..
cmake --build .
```

Binaries are placed into `build_debug/output`.

By default, klogg will rely on cmake to figure out target MacOS version. Usually it uses build host version.
To override default cmake value pass an option `-DKLOGG_OSX_DEPLOYMENT_TARGET=<target>` to cmake during configuration step,
`<target>` is one of `10.14`, `10.15`, `11`, `12`. Klogg's traget must be greater or equal to target used by Qt libraries.

## Running tests

Tests are built by default. To turn them off pass `-DBUILD_TESTS:BOOL=OFF` to cmake.
Tests use catch2 (bundled with klogg sources) and require Qt5Test module. Tests can be run using ctest tool provider by CMake:

```
cd <path_to_klogg_repository_clone>
cd build_debug
ctest --build-config RelWithDebInfo --verbose
```
