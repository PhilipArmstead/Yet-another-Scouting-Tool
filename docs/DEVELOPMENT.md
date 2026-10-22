# Development

## Requirements

Use a C17 compiler (GCC or Clang), CMake **3.22+**, Git and a desktop OpenGL 3.3 driver.
No GTK, GLib, GIO, Cairo, pkg-config GTK metadata or resource compiler is needed.
Raylib's bundled GLFW supplies the desktop window/input backend; audio and 3D models are disabled.

CMake automatically obtains these immutable release checkouts:

| Dependency | Checkout | Release | Commit |
|---|---|---|---|
| Raylib | `lib/raylib` | 6.2 | `02566453880e06bec97f5b8160fa6fe1e98397ca` |
| raygui | `lib/raygui` | 5.0 | `020a61bebcbe288b4414de3416e219ef40af847a` |
| utf8proc | `lib/utf8proc` | 2.12.0 | `79cdd5ab40c79afa559f689ffc13b76812dee1ac` |

First configuration needs network access. Matching local clones work offline. A mismatching
checkout fails explicitly and is never reset automatically; preserve any local work before
replacing it. Sources stay in `lib/`, while object files and libraries go in the chosen build tree.
These are local clones, not submodules. To upgrade, update both release and commit in
`cmake/dependencies.cmake` and rerun the regression suite.

### Linux

On Debian/Ubuntu:

```sh
sudo apt-get install build-essential cmake git pkg-config sysvinit-utils \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev libxkbcommon-dev wayland-protocols
```

Both X11 and Wayland are enabled by default. `pkg-config` is used by GLFW for system window
libraries, not for GTK. Equivalent Fedora packages include `gcc`, `cmake`, `git`, `pkgconf-pkg-config`,
`mesa-libGL-devel`, `libX11-devel`, `libXrandr-devel`, `libXinerama-devel`, `libXcursor-devel`,
`libXi-devel`, `wayland-devel`, `libxkbcommon-devel` and `wayland-protocols-devel`. On Arch, use
`base-devel cmake git pkgconf mesa libx11 libxrandr libxinerama libxcursor libxi wayland
libxkbcommon wayland-protocols`. Install your distribution's `pidof` provider for the live game
backend.

An X11-only build can use `-DGLFW_BUILD_WAYLAND=OFF`; a Wayland-only build can use
`-DGLFW_BUILD_X11=OFF`. The application needs a desktop session at runtime.

### macOS

Install Xcode Command Line Tools (`xcode-select --install`), CMake and Git. Apple Clang and
the system Cocoa/OpenGL frameworks are sufficient; no Homebrew GUI libraries are required.
The build produces a branded `YaST.app` bundle. This supports the application, not a new
native macOS FM24 memory reader.

### Windows

In an [MSYS2](https://www.msys2.org/) **MinGW64** terminal:

```sh
pacman -S --needed git mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja
```

Configure with `-G Ninja`, using that environment throughout. No MSYS2 GUI runtime is required.
NSIS is optional (`mingw-w64-x86_64-nsis` or an official NSIS installation with `makensis` on PATH).
Other native compilers must provide C17 atomics, designated initializers and alignment support.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DYAST_USE_LOCAL_CONFIG=OFF
cmake --build build --parallel
cmake --build build --target run
ctest --test-dir build --output-on-failure
```

Use a separate directory and `-DCMAKE_BUILD_TYPE=Debug` for Debug. Multi-config generators use
`cmake --build build --config Release` and `ctest --test-dir build -C Release`.
`cmake --build build --target info` reports the compiler and executable location.

`YAST_USE_LOCAL_CONFIG` defaults ON for existing developer setups. It includes an ignored
`src/app/config.h` when present; reproducible builds should turn it OFF. `YAST_MOCKS_MODE=ON`
selects deterministic synthetic players, clubs and nations without accessing a game process:

```sh
cmake -S . -B build-mock -DCMAKE_BUILD_TYPE=Debug \
  -DYAST_MOCKS_MODE=ON -DYAST_USE_LOCAL_CONFIG=OFF -DYAST_UI_TESTS=ON
cmake --build build-mock --parallel
ctest --test-dir build-mock --output-on-failure
```

`BUILD_TESTING` enables core tests. `YAST_UI_TESTS` is opt-in because it opens a native window.
It replays Raylib input events inside the process, rather than injecting desktop-wide keystrokes.
The tests cover calculation fixtures, sorting, snapshots, filters, Unicode matching, bounded
workers, native notifications, focus, UTF-8 deletion, table navigation and comparison ownership.
On headless Linux, run graphical tests under `xvfb-run -a`.

A mock executable also supports `--smoke-test [output-directory]`. It walks search, player details,
comparison, Best XI, depth, both themes and a smaller window, optionally saving PNGs. Invoke the
executable directly (`build-mock/YaST`, `build-mock/YaST.exe`, or
`build-mock/YaST.app/Contents/MacOS/YaST`). Use an isolated HOME/XDG directory on Unix when testing
configuration migration; otherwise this is a normal application that uses your user settings.

For sanitizer builds with GCC/Clang, configure a separate Debug tree with
`-DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"` and
`-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"`, then run CTest.

## Install and package

```sh
cmake --install build --prefix /path/to/staging
```

Linux installs `bin/YaST`, the desktop entry, scalable app icon and
`share/yast/licenses/`. macOS installs `YaST.app`, including dependency/font licenses inside
`Contents/Resources/licenses`. Images and fonts are embedded in the executable: launch directory
and external asset search paths do not affect rendering.

On Windows, `cmake --build build --target package` creates a CPack ZIP and, when `makensis` is
available at configure time, an NSIS installer. The installer is per-user under
`%LOCALAPPDATA%\Programs\YaST`. Packages contain `bin/YaST.exe`, necessary MinGW runtime DLLs
and `share/yast/licenses`; they do not contain a GTK runtime, schemas or loader caches.
Do not distribute a bare MinGW executable unless its required runtime DLLs are available.
User configuration/logs live outside the install tree and are not removed by an uninstall.

## Source layout and runtime

`yast_core` contains the existing readers/model, search, analysis and portable services.
`yast_ui` contains the immediate-mode frontend and generated embedded resource table.
`src/app/ui/controls.c` is the sole raygui implementation translation unit. Tables keep sorted
indices and draw visible rows only; analysis recomputes when its inputs change.

Cache jobs use a fixed worker set, staged allocations and main-thread publication. Autocomplete
has one coalescing worker per field and copied lookup data. Injury interning is synchronized.
Shutdown joins workers before closing game process handles or freeing shared state. POSIX
threads/Win32 services are used because Raylib has no general-purpose threading API.

The loop polls the existing game backend once per second and drains completed work and native
file notifications without waiting indefinitely for input. Frame caps are 60 focused, 15 inactive
and 5 minimized. Configuration paths, import precedence and hot reload are documented in
[OPTIONS.md](OPTIONS.md).

Fonts and PNGs are embedded by `cmake/resources.cmake`. Noto covers ordinary UI/Latin/Greek/
Cyrillic text; additional codepoints lazily build a Unifont atlas when data/input changes.
No glyph discovery, cache search, sorting or analysis runs unconditionally per frame.
See [CREDITS.md](CREDITS.md) for pinned font sources and licenses.

Existing SVG silhouettes are rasterized into checked-in PNGs, not interpreted at runtime.
To regenerate them, use `rsvg-convert`; to regenerate platform branding, run
`assets/branding/generate.sh` (requires `rsvg-convert`, Python and macOS `iconutil` for ICNS).
No asset-generation tools are needed for ordinary builds.

## Game access

Steam FM24 **24.4.2** is the existing target. The readers still depend on their platform's process
discovery, permissions and hard-coded offsets. The Linux reader expects its existing
Wine/Proton process layout and `pidof`; Windows needs rights to inspect the running process.
Application compilation or mock checks are not proof of live-game compatibility. No native
macOS FM24 offsets/discovery backend was added.
