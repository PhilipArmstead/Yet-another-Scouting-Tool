Yet another Scouting Tool
===

## About this tool

This application shows you player data from your running Football Manager 24 save and presents quick ratings so you can
compare players and weigh up future purchases.

This has only been tested with
the [Steam version of Football Manager 24](https://store.steampowered.com/app/2252570/Football_Manager_2024/), v24.4.2.

> **Officially supported platforms:** Linux and Windows (MSYS2/MinGW-w64).
>
> macOS builds may be possible for local development, but macOS is not an
> officially supported runtime platform.

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

### Common Build Requirements

- **C compiler**: A C99-capable GCC or Clang compiler. MinGW-w64 GCC is used on Windows.
- **Build system**: CMake 3.20 or newer
- **Package metadata**: `pkg-config` (provided by `pkgconf` on macOS and Arch Linux) so CMake can find `gtk4` and `gio-2.0`
- **GUI framework**: GTK4 development libraries
- **GLib tools**: `glib-compile-resources`, normally included with the GLib development package

### Linux

#### Debian/Ubuntu (and derivatives)

```bash
sudo apt-get install \
  build-essential \
  cmake \
  pkg-config \
  libgtk-4-dev \
  libglib2.0-dev \
  sysvinit-utils
```

`sysvinit-utils` provides `pidof`, which the Linux process backend uses to locate
the Football Manager process.

#### Red Hat/CentOS/Fedora

```bash
sudo dnf install \
  gcc \
  cmake \
  pkgconf-pkg-config \
  gtk4-devel \
  glib2-devel \
  procps-ng
```

#### Arch Linux

```bash
sudo pacman -S --needed \
  base-devel \
  cmake \
  pkgconf \
  gtk4 \
  procps-ng
```

If `pidof` is not available, install the process-utilities package for your
distribution. The exact package name may differ between distributions.

### macOS (Unofficial Local Builds Only)

macOS is not an officially supported runtime platform. These instructions are
provided only for contributors who want to experiment with local builds.
Install the Xcode Command Line Tools for Apple Clang, then install the build
dependencies with [Homebrew](https://brew.sh/):

```bash
xcode-select --install
brew install cmake pkgconf gtk4
```

The Homebrew `pkgconf` formula provides the `pkg-config` command. On Apple
Silicon, Homebrew normally uses `/opt/homebrew`; on Intel Macs it normally uses
`/usr/local`. Make sure the relevant Homebrew `bin` directory is on `PATH`
before configuring CMake.

### Windows

#### MSYS2/MinGW64 (Recommended)

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open the **MinGW64** terminal and run:

```bash
pacman -S --needed \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-gtk4
```

Use the same MinGW64 environment for CMake, the compiler, `pkg-config`, and the
GTK4 libraries. Do not mix MSYS, MinGW64, and native Windows package
environments. Other MinGW-w64 distributions can work if they provide matching
GTK4, GLib, and `pkg-config` installations.

## Building the Project

Run these commands from the repository root after installing the dependencies.

### Single-config generators

```bash
# Configure (Release is the default for single-config generators)
cmake -S . -B build

# Build
cmake --build build --parallel

# Build Debug
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel

# Clean build artifacts
cmake --build build --target clean
```

### Multi-config generators

```bash
# Configure once
cmake -S . -B build

# Build Debug
cmake --build build --config Debug --parallel

# Clean Debug build artifacts
cmake --build build --config Debug --target clean
```

### Run and inspect the build

The executable filename is derived from CMake's `PROJECT_NAME`. Prefer the
custom `run` target because it resolves the correct path for single-config
builds, multi-config builds, and application bundles produced by unofficial
macOS builds:

```bash
# Single-config generators
cmake --build build --target run

# Multi-config generators
cmake --build build --config Debug --target run

# Show compiler and target information
cmake --build build --target info
```

## Runtime Requirements

- GTK4 and GLib runtime libraries must be available to the application. The
  development packages normally install these automatically. If distributing
  a Windows build outside MSYS2, distribute matching GTK4/GLib DLLs and their
  dependencies.
- A graphical desktop session is required on the officially supported
  platforms: X11 or Wayland on Linux, or a Windows desktop session on Windows.
  Unofficial macOS builds require the macOS window server.
- Football Manager 24 must be running with the target save loaded. The current
  platform backends look for a process/module associated with `fm.exe`.
- The application must have permission to inspect and modify the Football
  Manager process. Linux systems may restrict `/proc/<pid>/mem` or ptrace
  access; Windows may require the appropriate process rights. Unofficial macOS
  builds may require additional process-access or debugging permissions
  depending on system security settings.
- On Linux, the `pidof` command must be available because the process-discovery
  backend uses it.

## Notes

- CMake selects the compiler from your environment or toolchain. Override it
  with `CC` or `-DCMAKE_C_COMPILER=...` when needed.
- `PROJECT_NAME` in `CMakeLists.txt` drives the application name passed to the
  code and the executable output name. `PROJECT_VERSION` drives the version
  shown in the footer.
- Platform-specific compile definitions are set automatically:
  `ARCH_LINUX` or `ARCH_WIN` for officially supported platforms. The code also
  defines `ARCH_MACOS` for unofficial local macOS builds.
- CMake compiles the UI layouts, CSS, icons, and flags into a generated
  GResource during the build. The generated files live in the build directory
  and should not be edited manually.
- On Windows, use a correctly configured MSYS2 MinGW64 environment for the
  whole build. The requirement is the matching toolchain and package paths,
  not a particular terminal application.
- GTK4 also requires the platform's normal graphics and font libraries. These
  are installed as transitive dependencies by the packages above.

## Troubleshooting

**"pkg-config not found"**

- Install the platform package listed above (`pkg-config`, `pkgconf`, or
  `mingw-w64-x86_64-pkgconf`) and ensure its directory is on `PATH`.
- Verify the command is available:

  ```bash
  pkg-config --version
  ```

**"gtk/gtk.h: No such file or directory"**

- Install the GTK4 development package for your platform.
- Check that CMake's package environment can see GTK4:

  ```bash
  pkg-config --modversion gtk4
  pkg-config --cflags --libs gtk4
  ```
- For unofficial macOS builds, make sure Homebrew's `pkgconf` and `gtk4`
  prefixes are on `PATH`/`PKG_CONFIG_PATH`. On Windows, run these commands
  from the matching MSYS2 MinGW64 environment.

**"ld.exe: cannot find -lgtk-4" (Windows)**

- Use the MinGW64 environment consistently and install
  `mingw-w64-x86_64-gtk4` and `mingw-w64-x86_64-pkgconf`.
- Do not mix libraries discovered by MSYS2 with a different compiler or
  architecture.

**CMake reports an imported target contains a non-existent include or library path**

- A package manager upgrade may leave the CMake cache pointing at an older
  dependency directory. With CMake 3.24 or newer, reconfigure with a fresh
  cache:

  ```bash
  cmake --fresh -S . -B build
  ```

- If the problem persists, remove only the affected build directory and
  configure it again. Do not reuse a cache after changing Homebrew prefixes or
  switching MSYS2 environments.

**"`glib-compile-resources` not found"**

- Install the GLib development package and verify that the command is on
  `PATH`.
- Verify that `gio-2.0` is visible to `pkg-config`:

  ```bash
  pkg-config --modversion gio-2.0
  ```

**"Football Manager process not found" or memory access is denied**

- Start Football Manager 24 and load the save before starting the application.
- Confirm that the process/module is exposed under the expected `fm.exe` name.
- Check the platform-specific process permissions described in Runtime
  Requirements.

**The application cannot start because no display is available**

- Run it from an active Linux X11/Wayland or Windows desktop session. An
  unofficial macOS build requires a macOS desktop session. A headless shell
  alone is not sufficient.

**Build fails with compiler errors**

- Confirm that the selected compiler supports C99 and that CMake is using the
  intended compiler/toolchain.
- After changing compilers or package environments, configure a fresh build
  directory rather than reusing the previous cache.
