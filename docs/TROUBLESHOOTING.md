## Troubleshooting

**Cannot find a supported running process 'fm.exe'**

- Start Football Manager 24 and load the target save before starting the application.
- Application support on macOS does not provide a native macOS FM24 reader. The retained backend
  targets its existing `fm.exe` layout, just as before the UI migration.
- Different game versions/offsets are not made compatible by switching UI toolkits.

**Memory access is denied**

- Make sure the application has permission to inspect and modify the Football Manager process. Linux systems may
  restrict `/proc/<pid>/mem` or ptrace access, while Windows may require the appropriate process rights.
- On Linux, make sure the `pidof` command is available.

**Options cannot be read, created or watched**

- Check the per-user paths in [OPTIONS.md](OPTIONS.md) and the log shown there. Configuration
  does not live in the launch directory anymore.
- An existing per-user file takes priority over an executable-adjacent legacy file.
- Native notifications follow file replacement, but moving/deleting the entire config directory
  requires restarting the app. Keep settings on a filesystem supporting native notifications.
- An unreadable/missing reload leaves the current settings in memory. Initial load failure uses
  in-memory defaults and reports a persistent status; correct the path/permissions and restart.

**Window/graphics initialization fails**

- A graphical desktop and OpenGL 3.3 driver are required. Update the graphics driver or check
  software-renderer availability. Linux headless graphical checks need Xvfb.
- For source builds, check the X11/Wayland development dependencies in
  [DEVELOPMENT.md](DEVELOPMENT.md). There is no GTK runtime to install.

**Some international characters are missing**

- Bundled Noto Sans and Unifont cover many scripts, but not every Unicode codepoint. Missing
  glyphs are reported in the status/log. Complex shaping, bidi and IME composition are outside
  the supported text model.

**Windows package does not start**

- Run `bin/YaST.exe` from the complete extracted package, with its runtime DLLs beside it.
- Build the `package` target instead of copying only the compiler's output executable.