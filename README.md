Yet another Scouting Tool
===

## About this tool

This application reads player data from your running Football Manager 24 save and presents quick ratings so you can
compare players and weigh up future purchases.

It has only been tested with
the [Steam version of Football Manager 24](https://store.steampowered.com/app/2252570/Football_Manager_2024/), v24.4.2.

> **Officially supported platforms:** Linux and Windows.
>
> macOS is not an officially supported runtime platform.

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

## Troubleshooting

**Cannot find running process 'fm.exe'**

- Start Football Manager 24 and load the target save before starting the application.

**Memory access is denied**

- Make sure the application has permission to inspect and modify the Football Manager process. Linux systems may
  restrict `/proc/<pid>/mem` or ptrace access, while Windows may require the appropriate process rights.
- On Linux, make sure the `pidof` command is available.

## Development

For build requirements, source-build instructions, and developer troubleshooting, see
the [development guide](docs/DEVELOPMENT.md).
