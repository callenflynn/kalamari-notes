# Kalamari Notes

![Kalamari Banner](src/assets/kalamari_banner.png)

A fast, cross-platform markdown notebook built with C++, SDL3, and Dear ImGui.

## Prerequisites

- **CMake** 3.28+ (`winget install Kitware.CMake`)
- **Visual Studio 2026** with "Desktop development with C++" workload
- **WiX Toolset** (for MSI installer): `winget install WiXToolset.WiX`

## Build (Windows)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

## Package (Windows)

```powershell
cd build
cpack -C Release
```

Outputs `Kalamari-*.msi` and `Kalamari-*.zip` in `build/`.

## Build (macOS / Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cd build && cpack -C Release
```

## Release

Push a tag to trigger the CI build and create a GitHub Release with installers:

```powershell
git tag v1.0.0
git push origin v1.0.0
```
