Yet another Scouting Tool
===

## About this tool

This application shows you player data from your running Football Manager 24 save and presents quick ratings so you can
compare players and weigh up future purchases.

This has only been tested with
the [Steam version of Football Manager 24](https://store.steampowered.com/app/2252570/Football_Manager_2024/), v24.4.2.

> **Supported platforms:** Linux and Windows only.

### Features

#### Analyse players

- See all the attributes for any player (including hidden and personality) as well as a competency rating for all
  comfortable positions
- Remove injuries for a given player and improve their condition + sharpness
- Boost player's morale

#### Search for players

- Search based on a number of factors including age, current/potential ability, position and rating

#### Analyse a club

- List all players for all squads (senior, reserves, U18)
- Pick the best XI for a given formation based on player competency ratings
- See squad depth options and identify weaknesses in positions

## Dependencies

### Common Requirements (All Platforms)

- **C Compiler**: GCC 9+ or Clang 10+ (Linux), or MinGW-w64 GCC (Windows), with C99 support
- **Build System**: CMake 3.20+
- **Package Manager**: pkg-config (for GTK4 detection)
- **GUI Framework**: GTK4 development libraries

### Linux

#### Debian/Ubuntu (and derivatives)

```bash
sudo apt-get install \
  build-essential \
  pkg-config \
  cmake \
  libgtk-4-dev \
  libglib2.0-dev
```

**Packages**:

- `build-essential` - GCC compiler, make, libc development files
- `pkg-config` - Package configuration helper
- `libgtk-4-dev` - GTK4 development headers and libraries
- `libglib2.0-dev` - GLib development headers (GTK4 dependency)

#### Red Hat/CentOS/Fedora

```bash
sudo dnf install \
  gcc \
  cmake \
  pkg-config \
  gtk4-devel \
  glib2-devel
```

**Packages**:

- `gcc` - GNU C Compiler
- `make` - Build automation tool
- `pkg-config` - Package configuration helper
- `gtk4-devel` - GTK4 development headers and libraries
- `glib2-devel` - GLib development headers (GTK4 dependency)

#### Arch Linux

```bash
sudo pacman -S \
  base-devel \
  pkg-config \
  cmake \
  gtk4
```

**Packages**:

- `base-devel` - Core development tools (GCC, make, libc)
- `pkg-config` - Package configuration helper
- `gtk4` - GTK4 libraries and headers

### Windows

#### MSYS2/MinGW64 (Recommended)

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open MinGW64 terminal and run:

```bash
pacman -S \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-gtk4
```

**Packages**:

- `mingw-w64-x86_64-toolchain` - GCC compiler and build tools
- `mingw-w64-x86_64-cmake` - CMake build system
- `mingw-w64-x86_64-pkg-config` - Package configuration helper
- `mingw-w64-x86_64-gtk4` - GTK4 libraries and headers for Windows 64-bit

**Alternative: MinGW-w64**

- Standalone MinGW-w64 distribution with pkg-config and GTK4 support

**Note**: The build files include Windows-specific platform detection (`-DARCH_WIN` flag).

## Building the Project

### On Any Platform (after dependencies are installed)

```bash
# Configure (Release is the default for single-config generators)
cmake -S . -B build

# Build
cmake --build build

# Build Debug on single-config generators (Unix Makefiles, Ninja)
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Build Debug on multi-config generators (Visual Studio)
cmake --build build --config Debug

# Build with multiple cores
cmake --build build --parallel

# Clean build artifacts
cmake --build build --target clean

# Run directly
./build/YaST   # single-config generators
# or
./build/Debug/YaST   # multi-config generators, Debug

# Or use the custom run target
cmake --build build --target run
# (multi-config)
cmake --build build --config Debug --target run

# Show build info (via CMake custom target)
cmake --build build --target info
```

## Runtime Requirements

- **GTK4 Runtime Libraries** (usually included with development packages)
- **X11 or Wayland** (Linux display server)
- **GLib2 Runtime** (included with GTK4)

### X11 on Linux (if not already running Wayland)

Most modern distributions include X11. For headless systems or when needed:

- `libx11-6` (Debian/Ubuntu)
- `libx11` (Fedora/RHEL)
- `libx11` (Arch)

## Notes

- CMake selects the compiler from your environment/toolchain. Override with `CC` or `-DCMAKE_C_COMPILER=...` when
  needed.
- Platform-specific flags are set automatically during build:
	- Linux: `-DARCH_LINUX`
	- Windows: `-DARCH_WIN`
- On Windows, ensure you're using the **MinGW64 shell** (not CMD.exe or PowerShell)
- GTK4 requires a modern C library and platform-specific graphics libraries

## Troubleshooting

**"pkg-config not found"**

- Install pkg-config package for your platform (see above)

**"gtk/gtk.h: No such file or directory"**

- Install GTK4 development headers (`libgtk-4-dev` or equivalent)

**"ld.exe: cannot find -lgtk-4" (Windows)**

- Ensure you're using the MinGW64 shell with GTK4 pacman package installed

**Build fails with compiler errors**

- Ensure your C compiler supports C99 standard (GCC 9+, Clang 10+)
