# Yet another Scouting Tool

Cross-platform C17 desktop app using Raylib 6.2 and raygui 5.0. Reads live memory from Football Manager 24 to search the player
database, show hidden/calculated attributes, and build the best starting 11 from search results.

Priorities, in order: performance, correctness, readability.

Dependency sources are pinned clones under `lib/`; compiled output belongs in CMake build directories.
UI state is plain C, drawing is immediate-mode, and tables render only visible rows. Cache and autocomplete
workers publish on the main thread. Keep snapshots independent of cache allocations and join workers before
closing process handles or freeing shared data.
