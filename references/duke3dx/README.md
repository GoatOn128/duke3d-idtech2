# Duke3DX

**Duke3DX** is a cleanroom reimplementation of the original Build engine used by Duke Nukem 3D, while preserving the original Duke Nukem 3D 1.5 game code with minimal modifications, ported to modern C++ and Win32.

---

## Overview

Duke3DX aims to provide a faithful, source-accurate version of Duke Nukem 3D by:

- Keeping the original game logic intact
- Replacing the original Build engine implementation with a cleanroom version
- Porting the codebase to modern C++
- Running natively on Win32 systems without DOS dependencies

The goal is **accuracy first**, with modernization only where required for compatibility and stability.

---

## Key Features

- Cleanroom implementation of the Build engine
- Original Duke Nukem 3D 1.5 game code preserved
- C++ port of legacy C/DOS codebase
- Win32-native (no DOSBox required)
- Minimal behavioral changes from vanilla gameplay
- Designed for further rendering and engine experimentation

---

## Project Goals

- Maintain 1:1 gameplay behavior with the original
- Remove DOS-specific dependencies
- Provide a stable base for modern rendering backends
- Enable easier debugging, profiling, and extension

---

## Non-Goals

- No gameplay changes or enhancements
- No rebalancing or modding features (out of scope)
- No deviation from original mechanics unless required for correctness

---

## Requirements

- Windows (Win32)
- CMake (recommended)
- Visual Studio (or compatible C++ compiler)

---

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
