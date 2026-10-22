Yet another Scouting Tool
===

# About

This app reads player data from your running Football Manager 24 game and lets you search the player database. It also
rates players according to customisable weights.

It has only been tested with
the [Steam version of Football Manager 24](https://store.steampowered.com/app/2252570/Football_Manager_2024/), v24.4.2.

The C17 desktop application uses **Raylib 6.2 + raygui 5.0** and builds on Windows, Linux and macOS.
The existing game readers target `fm.exe`; building the macOS application does **not** add a native
macOS FM24 reader. Game compatibility remains limited to the existing backends and offsets.

Screenshots below show synthetic data from the mock build.

## Features

### Search for players

- Search based on a number of factors:
	- age
	- position
	- nationality
	- current/potential ability
	- rating
	- club

![Screenshot of player search](docs/player-search.png?v=2)

### Analyse players

- See all the attributes for any player (including hidden and personality) as well as a competency rating for all
  comfortable positions
	- These competencies can be customised with either universal or per-position weights

![Screenshot of player info](docs/player-info.png?v=2)

Player details and analysis open in closable tabs. Use **Compare** to pin player details alongside
another player, the search table or an analysis. Views own snapshots and survive cache refreshes.

### Analyse a squad

- Assemble the best XI from all players in search results, for a given formation, based on player competency ratings
- View squad depth, outlining the strength of your options across all positions

![Screenshot of best XI](docs/best-xi.png?v=2)
![Screenshot of squad depth](docs/squad-depth.png?v=2)

### Keyboard and text

Tab/Shift-Tab moves focus; Enter/Space activates controls. Enter searches from a filter or opens the
selected table row. Arrow, Page Up/Down, Home and End keys navigate the table; Left/Right scroll its
columns. Shift-click a sortable header to add a secondary sort. Autocomplete supports Up/Down,
Enter and Escape. Ctrl/Cmd+A/C/X/V selects, copies, cuts and pastes in text fields; Ctrl/Cmd+W
closes the active tab and Ctrl/Cmd+Q quits. Shift-wheel scrolls horizontally. Focus a scrollbar
with Tab to use arrows, Page Up/Down, Home and End to reach clipped controls or comparison panels.

International UTF-8 names and accent-insensitive autocomplete are supported through bundled fonts
and utf8proc. Complex-script shaping, bidirectional layout, IME composition and native
screen-reader integration are not provided by this immediate-mode frontend.

## Options

The app can be customised by modifying the per-user **options.yaml** file, which is created on the first run. The options file is
subject to a "hot-reload" meaning changes will be reflected immediately without having to restart the app.

See the [options guide](docs/OPTIONS.md) for more information.

An executable-adjacent legacy file is imported only when no per-user file exists; the original is
left untouched. Fonts and images are embedded, so launching from a different directory is safe.

# Feature request?

[Open an issue](https://github.com/PhilipArmstead/Yet-another-Scouting-Tool/issues), clearly labelled as a feature
request, and I'll consider adding it to the app.

# Troubleshooting

For troubleshooting information, see
the [troubleshooting guide](docs/TROUBLESHOOTING.md). If you struggle with an issue you can't
resolve, [open an issue](https://github.com/PhilipArmstead/Yet-another-Scouting-Tool/issues).

# Development

For build requirements, source-build instructions, and developer troubleshooting, see
the [development guide](docs/DEVELOPMENT.md).

# Credits

For attribution of all art assets in this project, see the [credits page](docs/CREDITS.md).

No generative AI was used in the production of any art in this project.

# Support

If you enjoy using this tool and think it's worth a cup of tea, consider kicking me a couple of quid. Any contributions
are **extremely** appreciated.

[![Support me on Ko-fi](docs/kofi.png?v=1)](https://ko-fi.com/philarmstead)