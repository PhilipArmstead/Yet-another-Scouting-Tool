Options
===

These are the available options, their default values and a brief description of what they do. The options file is "hot
reloadable" in this app, meaning changes will be reflected upon saving with no restart necessary.

## File location and migration

| Platform | Configuration | Log |
|---|---|---|
| Windows | `%APPDATA%\YaST\options.yaml` | `%LOCALAPPDATA%\YaST\yast.log` |
| Linux | `$XDG_CONFIG_HOME/yast/options.yaml`, otherwise `~/.config/yast/options.yaml` | `$XDG_STATE_HOME/yast/yast.log`, otherwise `~/.local/state/yast/yast.log` |
| macOS | `~/Library/Application Support/YaST/options.yaml` | `~/Library/Logs/YaST/yast.log` |

The app creates missing directories and defaults. If the per-user file does not exist, it first
copies an `options.yaml` beside the executable. Existing per-user configuration always wins;
the legacy original is never changed or deleted. On macOS, "beside the executable" means
`YaST.app/Contents/MacOS`, not the directory containing the bundle.

Saves are detected through native filesystem notifications (inotify, kqueue or
`ReadDirectoryChangesW`) and debounced for 120 ms. Atomic editor replacements and delete/recreate
saves are followed. A temporarily missing/unreadable file leaves current settings in memory and
reports an error. A moved/deleted configuration directory requires restarting the app.
Weights and themes update existing snapshots, search results and analyses.

The parser accepts the documented YAML subset, not arbitrary YAML. Invalid entries are logged;
formations require exactly eleven valid position codes. An empty formation list leaves Best XI
unavailable instead of crashing. Weights must be finite, scales finite and positive; negative
weights are allowed. The options file does not need write access beside the application.

### Dark mode

Set to `true` to enable dark mode. **Default value** is `dark-mode: false`

### Formations

The formations displayed in the "Best XI" view. Rearrange the list or modify it to suit your tactical needs. **Default
value** is:

```
formations:
  - name: "4-4-2"
    positions: GK DL DC DC DR ML MC MC MR ST ST
  - name: "4-3-3"
    positions: GK DL DC DC DR MC MC MC AML AMR ST
  - name: "4-2-3-1"
    positions: GK DL DC DC DR DM DM AML AMC AMR ST
  - name: "4-2-4 IF"
    positions: GK DL DC DC DR DM DM AML AMR ST ST
```

### Ratings

The weights used to calculate players' role ratings. Each object follows this structure:

```
ratings:
  - position: GK
    scale: 1.10
    weights:
      - Determination: 20.00
        Consistency: 18.53
        Reflexes: 19.35
        AerialReach: 6.45
        # and so on...
```

#### Position

Below is a list of allowed values for the `position` field:

- GK
- FB
- CB
- WB
- DM
- MC
- W
- AM
- ST
- General

Players' ratings are scored against all the positions listed above (assuming they can play them). If a position is
missing from the `ratings` open, the "General" rule is used instead. You can use this to define a single group of
weights for all outfield players.

#### Scale

The weighted sum is divided by the positive `scale` to normalise its display range.

#### Weights

A list of optional attributes and the weighting applied to them.

Ratings are calculated by converting each raw attribute to a score between 0 and 1, multiplying it by the weight, and
summing all the weighted scores. The final value is then divided by `scale`.

Any attribute missing from the weight list is ignored. The following attributes are available for weighting:

- Crossing
- Dribbling
- Finishing
- Heading
- LongShots
- Marking
- OffTheBall
- Passing
- PenaltyTaking
- Tackling
- Vision
- Handling
- AerialReach
- CommandOfArea
- Communication
- Kicking
- Throwing
- Anticipation
- Decisions
- OneOnOnes
- Positioning
- Reflexes
- FirstTouch
- Technique
- LeftFoot
- RightFoot
- Flair
- CornerTaking
- Teamwork
- WorkRate
- LongThrows
- Eccentricity
- RushingOut
- Punching
- Acceleration
- FreeKickTaking
- Strength
- Stamina
- Pace
- JumpingReach
- Leadership
- Dirtiness
- Balance
- Bravery
- Consistency
- Aggression
- Agility
- ImportantMatches
- InjuryProneness
- Versatility
- NaturalFitness
- Determination
- Composure
- Concentration
- Adaptability
- Ambition
- Loyalty
- Pressure
- Professionalism
- Sportsmanship
- Temperament
- Controversy