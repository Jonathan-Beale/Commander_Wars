# Hex map format (`*.hexmap.json`)

Hex maps for the Commander Wars hex prototype are written as a small JSON text file.
Save it as `maps/<N>_player/<Map Name>.hexmap.json` (for example `maps/2_player/Hex River.hexmap.json`).
The game converts it to its own binary `.map` file the next time it starts, and again whenever the text file is newer.

Four finished examples ship with the prototype:

- `maps/2_player/Hex Test.hexmap.json`
- `maps/2_player/Hex River.hexmap.json`
- `maps/3_player/Hex Triad.hexmap.json`
- `maps/4_player/Hex Quad.hexmap.json`
- `maps/6_player/Hex Crown.hexmap.json` (a large hexagon, written by `hex_balance/make_six_player_map.py`)

## Example

```json
{
    "name": "Tiny Example",
    "description": "Two bases on either side of a river.",
    "author": "Your name",
    "players": 2,
    "width": 7,
    "height": 5,
    "legend": {".": "PLAINS", "F": "FOREST", "M": "MOUNTAIN", "~": "RIVER", "=": "STREET", "B": "BRIDGE1", "S": "SEA"},
    "terrain": [
        ". . F ~ . . .",
        " . . . ~ . M .",
        "  . = = B = = .",
        "   . M . ~ . . .",
        "    . . . ~ F . ."
    ],
    "buildings": [
        {"id": "HQ",      "x": 0, "y": 2, "owner": 1},
        {"id": "FACTORY", "x": 0, "y": 1, "owner": 1},
        {"id": "HQ",      "x": 6, "y": 2, "owner": 2},
        {"id": "FACTORY", "x": 6, "y": 3, "owner": 2},
        {"id": "TOWN",    "x": 2, "y": 0, "owner": 0}
    ],
    "units": [
        {"id": "INFANTRY",   "x": 1, "y": 2, "owner": 1},
        {"id": "LIGHT_TANK", "x": 5, "y": 2, "owner": 2}
    ]
}
```

## The grid

- The map is a grid of hexagons with pointed tops, `width` columns by `height` rows.
- Coordinates are `x` (column, starting at 0 on the left) and `y` (row, starting at 0 at the top).
- **Each row sits half a hex further right than the row above it.** A rectangular grid of coordinates is therefore
  drawn as a parallelogram leaning to the right. Indent row `y` by `y` spaces in the text, as in the example,
  and the text shows the map the way the game draws it.
- Every hex has six neighbours. For the hex at `(x, y)` they are:
  left `(x-1, y)`, right `(x+1, y)`, upper left `(x, y-1)`, upper right `(x+1, y-1)`, lower left `(x-1, y+1)`,
  lower right `(x, y+1)`.
- The distance between two hexes is `(|dx| + |dy| + |dx + dy|) / 2`, where `dx` and `dy` are the differences of the
  coordinates. Unit movement, attack ranges and vision all use this distance.
- To get a board that is not a parallelogram (a hexagon, a ring, a ragged outline), mark the unused hexes with `X`.
  An `X` hex has no tile: it is not drawn, nothing can enter it, and the game crops the view and the minimap to the
  hexes that do exist. `Hex Triad` and `Hex Crown` are hexagons made this way. Use `SEA` only where you want real
  water that ships can sail on.

## Rectangle layout

If you would rather design on a plain rectangle, add `"layout": "rectangle"`. Write every row starting at the same
column, with no indentation. When the map is loaded, the game shifts the rows alternately right and left by half a hex,
so the board keeps its rectangular outline with zigzag sides.

- Coordinates in `buildings` and `units` are the row and column as written.
- The six neighbours of a hex then depend on the row. In an even row (0, 2, 4, ...) the hex at column `c` touches
  columns `c-1` and `c` in the rows above and below. In an odd row it touches columns `c` and `c+1`.
  In both cases it also touches `c-1` and `c+1` in its own row.
- Symmetry on this layout: mirroring top to bottom is exact when the height is odd. Rotating half a turn about the
  centre is exact when the height is even. Mirroring left to right is never exact, because odd rows sit half a hex to
  the right; the two halves end up half a hex apart.
- `X` (no tile) works here as well.

Maps made for the original square game are treated the same way automatically when the hex grid is on.

## Wrapping maps (globe)

Add `"wrap": true` to make the map wrap around east to west: a unit leaving the right edge arrives on the left edge
of the same row, and ranges, vision and the AI all work across the seam. In the game the camera scrolls sideways
without end.

- Write the rows as in the rectangle layout: every row starts at the same column, coordinates in `buildings` and
  `units` are the row and column as written, and the neighbour rule is the same. `layout` is not needed.
- The last column of a row touches the first column of that row and of the rows above and below, following the same
  rule (column `width` is column 0).
- Do not use `X` on a wrapping map. Use `SEA` or mountains for the top and bottom rows if you want polar barriers;
  the map does not wrap north to south.
- Make the map at least about 40 columns wide if you do not want players to see the same place twice on a wide
  screen when zoomed out.
- A layout repeated every `width / players` columns gives every player an identical position. `Hex Globe`
  (`maps/4_player`, written by `hex_balance/make_wrap_map.py`) is built this way.

## Fields

| Field | Required | Meaning |
|---|---|---|
| `name` | no | Name shown in the game. Defaults to the file name. |
| `description` | no | Text shown in the map selection. |
| `author` | no | Map author. |
| `players` | yes | Number of players, 2 or more. |
| `width`, `height` | yes | Size of the grid in hexes. |
| `layout` | no | Leave it out for the hex layout described above. `"rectangle"` means the rows are written like a square-grid map, all starting at the same column; see "Rectangle layout". |
| `wrap` | no | `true` makes the map wrap around east to west; see "Wrapping maps (globe)". |
| `rules` | no | List of game rule ids switched on for this map, e.g. `["GAMERULE_CIVILIZATION_WARS"]`. |
| `legend` | no | Symbols used in `terrain`. See below. |
| `terrain` | yes | One text line per row, top row first. Each line holds `width` symbols separated by spaces. Extra spaces are ignored. |
| `buildings` | no | List of buildings: `id`, `x`, `y`, and `owner` (0 = neutral, 1 = player 1, ...). |
| `units` | no | List of starting units: `id`, `x`, `y`, `owner` (1 = player 1, ...), and optionally `hp` (1 to 10). |

### Legend

A legend entry maps a symbol to a terrain ID, or to an object with a terrain and a building:

```json
"legend": {
    "F": "FOREST",
    "T": {"building": "TOWN"},
    "1": {"building": "HQ", "owner": 1},
    "2": {"building": "HQ", "owner": 2}
}
```

These symbols work without a legend: `.` plains, `F` forest, `M` mountain, `~` river, `=` road, `B` bridge,
`S` sea, `T` neutral town, `X` no tile (edge of the map). A symbol can be longer than one character, as long as it contains no spaces.

## Key: every ID you can use

IDs are written in upper case exactly as shown. The name is what the game calls it.

Only the IDs marked **tested** have been played on the hex grid so far. The others load through the same code and
should work, but check them in the game before building a map around them.

### Terrain

Use these as legend values (`"F": "FOREST"`).

**Standard terrain**

| ID | Name | Notes |
|---|---|---|
| `PLAINS` | Plains | **Tested.** The default; every hex is plains unless the map says otherwise. |
| `FOREST` | Forest | **Tested.** Cover and a hiding place in fog. `FOREST1`, `FOREST2` and `FOREST3` are the same terrain with different tree art. |
| `MOUNTAIN` | Mountain | **Tested.** Foot units only; extra vision for infantry in fog. |
| `STREET` | Street | **Tested.** Road: cheapest movement, no cover. `STREET1` is the same with different art. |
| `RIVER` | River | **Tested.** Foot units only. |
| `BRIDGE1` | Bridge | **Tested.** Lets ground units cross a river or sea hex. Place it on the river hex. |
| `BRIDGE2` | Old bridge | Same as `BRIDGE1` with different art. |
| `BRIDGE` | Ship bridge | Bridge over river or sea; over sea, ships can still pass. |
| `SEA` | Sea | **Tested.** Ships and aircraft only. Also used to cut the board into a shape. |
| `ROUGH_SEA` | Rough Sea | Slows ships; aircraft are not affected. |
| `REAF` | Reef | Sea hex that hides ships in fog. (The ID is spelled `REAF`.) |
| `LAKE` | Lake | Inland water for ships and aircraft; hovercraft move on and off freely. |
| `FOG` | Fog | Sea hex that hides ships in fog. |
| `BEACH` | Beach | Shore hex where ships land ground troops. |
| `RUIN` | Ruin | Light cover and a hiding place in fog. |
| `WASTELAND` | Wasteland | Rough ground that slows vehicles. |
| `PIPELINE` | Pipeline | Wall that most units cannot cross; pipe runners travel along it. |
| `WELD` | Weld | Weak point in a pipeline that can be shot open. |
| `DESTROYEDWELD` | Destroyed weld | A weld that has been opened. |
| `WALL` | Wall | Most units cannot cross it. It can be opened at gates and weak wall hexes. |
| `WEAK_WALL` | Weak Wall | Wall hex that can be destroyed to open a path. |
| `METEOR` | Meteor | Can be attacked and destroyed, which removes the plasma attached to it. |
| `PLASMA` | Plasma | Impassable; disappears when its meteors are destroyed. |
| `PLAINS_DESTROYED` | Plains | Plains with crater art. |
| `PLAINS_PLASMA` | Plains | Plains with scorched art. |
| `FIRE` | Fire | Reveals the area around it in fog. |
| `CREEPER` | Creeper | Deals 1 HP of damage each day to a unit standing on it. |
| `TELEPORTTILE` | Teleport Tile | Costs no movement points to cross. |
| `VOID` | Edge of the map | **Tested.** No tile at all. Not drawn and impassable for every unit, including aircraft. Used to give a map a shape other than a parallelogram. |

**Biome variants.** These behave like the standard terrain of the same kind, with different art and the biome's
movement or firepower penalty.

| Kind | Desert | Snow | Waste |
|---|---|---|---|
| Open ground | `DESERT` | `SNOW` | `WASTE` |
| Forest | `DESERT_FOREST`, `DESERT_FOREST1` | `SNOW_FOREST`, `SNOW_FOREST1`, `SNOW_FOREST2` | `WASTE_FOREST` |
| Mountain | `DESERT_ROCK` | `SNOW_MOUNTAIN` | `WASTE_MOUNTAIN` |
| Road | `DESERT_PATH`, `DESERT_PATH1` | `SNOW_STREET` | `WASTE_PATH` |
| River | `DESERT_TRY_RIVER` (dry riverbed, most units can cross) | | |
| Ruin | `DESERT_RUIN` | `SNOW_RUIN` | `WASTE_RUIN` |
| Wasteland | `DESERT_WASTELAND` | `SNOW_WASTELAND` | `WASTE_WASTELAND` |
| Pipeline | `DESERT_PIPELINE` | `SNOW_PIPELINE` | `WASTE_PIPELINE` |
| Weld | `DESERT_WELD` | `SNOW_WELD` | `WASTE_WELD` |
| Destroyed weld | `DESERT_DESTROYEDWELD` | `SNOW_DESTROYEDWELD` | `WASTE_DESTROYEDWELD` |

### Buildings

Use these as `id` in the `buildings` list or as `building` in a legend entry.

**Standard buildings** (one hex each)

| ID | Name | Notes |
|---|---|---|
| `HQ` | HQ | **Tested.** One per player. Losing it loses the game. |
| `TOWN` | Town | **Tested.** Income and repairs for ground units. |
| `FACTORY` | Base | **Tested.** Builds ground units. |
| `AIRPORT` | Airport | Builds and repairs air units. |
| `HARBOUR` | Harbour | Builds and repairs ships. |
| `TOWER` | Tower | Boosts the owner's attack and defence. |
| `LABOR` | Labor | Builds and repairs Oozium units. |
| `RADAR` | Radar | Reveals the area around it in fog. |
| `SILO_ROCKET` | Silo | Missile silo that a foot unit can launch once. |
| `SILO` | Silo | An empty silo; gives some cover. |
| `MINE` | Mine | High income; cannot repair or resupply. |
| `OILRIG` | Oil Rig | High income; cannot repair or resupply. |
| `POWERPLANT` | Power plant | Makes the owner's CO power meter charge faster. |
| `DEPOT` | Depot | Cover, and partial resupply for ground units. |
| `PIPESTATION` | Pipestation | Builds and repairs pipe runners; also gives income. |
| `AMPHIBIOUSFACTORY` | Amphibious factory | Builds and resupplies hovercraft. |
| `FIELD_BASE` | Field Base | Acts like an HQ for winning and losing, but cannot build, repair or resupply. |
| `FORTHQ` | Fortified HQ | An HQ that is harder to capture. |
| `TEMPORARY_AIRPORT` | Temporary Airport | Built by an APC during play; repairs air units, no income or production. |
| `TEMPORARY_HARBOUR` | Temporary Harbour | The naval counterpart of the temporary airport. |

**Special one-hex structures.** These are neutral obstacles and weapons from the campaigns. Several point in a
compass direction (`_N`, `_E`, `_S`, `_W`), which was designed for the square grid and has not been adapted to hexes.

| ID | Name |
|---|---|
| `ZMINICANNON_N`, `ZMINICANNON_E`, `ZMINICANNON_S`, `ZMINICANNON_W` | Mini Cannon |
| `ZLASER` | Laser |
| `ZCRYSTALL` | Crystal |
| `ZVOLCAN`, `ZVOLCANDESERT`, `ZVOLCANSNOW`, `ZVOLCANWASTE` | Volcano |
| `ZBLACKHOLE_FACTORY`, `ZBLACKHOLE_FACTORYDESERT`, `ZBLACKHOLE_FACTORYSNOW`, `ZBLACKHOLE_FACTORYWASTE` | Black Hole factory |
| `ZGATE_E_W`, `ZGATE_N_S` | Gate |
| `ZDESTROYED_GATE_E_W`, `ZDESTROYED_GATE_N_S` | Destroyed gate |
| `ZWELD_E_W`, `ZWELD_N_S` | Weld |
| `ZDESERTWELD_E_W`, `ZDESERTWELD_N_S` | Desert Weld |
| `ZSNOWWELD_E_W`, `ZSNOWWELD_N_S` | Snowy Weld |
| `ZWASTEWELD_E_W`, `ZWASTEWELD_N_S` | Waste Weld |

**Not usable on hex maps.** These cover more than one hex or are parts of such a structure, which the hex prototype
does not support: every `ZBLACKHOLE_CANNON_*`, `ZDEATHRAY_*`, `ZGRAND_BOLT_*` and `ZNEST_*`, and `ZFORTRESS`,
`ZMONOLITH`, `ZOMEGA_BASE`, `ZTALON_GUN`, `ZTALON_GUN_DESTROYED`, `ZBLACK_BUILDING_DESTROYED`.

### Units

Use these as `id` in the `units` list. **Move** is the unit's movement points per turn, counted in hexes on a road.

**Ground units** (built at a `FACTORY`)

| ID | Name | Movement type | Move |
|---|---|---|---|
| `INFANTRY` | Infantry | Foot | 3 |
| `MECH` | Mech | Mech | 2 |
| `SNIPER` | Sniper | Foot | 3 |
| `MOTORBIKE` | Motorbike | Tires B | 5 |
| `RECON` | Recon | Tires A | 8 |
| `FLARE` | Flare | Treads | 5 |
| `APC` | APC | Treads | 6 |
| `LIGHT_TANK` | Light Tank | Treads | 6 |
| `HEAVY_TANK` | Heavy Tank | Treads | 5 |
| `NEOTANK` | Neotank | Treads | 6 |
| `MEGATANK` | Megatank | Treads | 5 |
| `ARTILLERY` | Artillery | Treads | 5 |
| `ANTITANKCANNON` | Anti Tank Cannon | Tires B | 4 |
| `ROCKETTHROWER` | Rockets | Tires A | 5 |
| `FLAK` | Anti Air | Treads | 6 |
| `MISSILE` | Missile | Tires A | 4 |
| `PIPERUNNER` | Piperunner | Pipeline only | 9 |
| `HOELLIUM` | Oozium | Oozium | 1 |

`INFANTRY`, `MECH`, `RECON`, `LIGHT_TANK` and `ARTILLERY` are **tested**.

**Hovercraft** (built at an `AMPHIBIOUSFACTORY`; move over both land and water)

| ID | Name | Move |
|---|---|---|
| `HOVERCRAFT` | Hovercraft | 6 |
| `HEAVY_HOVERCRAFT` | Heavy Hovercraft | 5 |
| `ARTILLERYCRAFT` | Artillerycraft | 5 |
| `HOVERFLAK` | Hover Anti Air | 6 |

**Air units** (built at an `AIRPORT`)

| ID | Name | Move |
|---|---|---|
| `T_HELI` | Transport Copter | 6 |
| `K_HELI` | Battle Copter | 6 |
| `DUSTER` | Duster | 8 |
| `FIGHTER` | Fighter | 9 |
| `BOMBER` | Bomber | 7 |
| `STEALTHBOMBER` | Stealthbomber | 7 |
| `TRANSPORTPLANE` | Transportplane | 7 |
| `WATERPLANE` | Waterplane | 7 |
| `BLACK_BOMB` | Black Bomb | 9 |

**Naval units** (built at a `HARBOUR`)

| ID | Name | Movement type | Move |
|---|---|---|---|
| `LANDER` | Lander | Boat | 6 |
| `BLACK_BOAT` | Black Boat | Small boat | 6 |
| `GUNBOAT` | Gun boat | Small boat | 7 |
| `CANNONBOAT` | Missile boat | Small boat | 7 |
| `TORPEDOBOAT` | Torpedo boat | Small boat | 6 |
| `FRIGATE` | Frigate | Boat | 7 |
| `DESTROYER` | Destroyer | Boat | 7 |
| `CRUISER` | Cruiser | Ship | 6 |
| `SUBMARINE` | Submarine | Ship | 6 |
| `BATTLECRUISER` | Battlecruiser | Ship | 6 |
| `BATTLESHIP` | Battleship | Ship | 5 |
| `AIRCRAFTCARRIER` | Aircraft Carrier | Ship | 5 |
| `WATERMINE` | Watermine | Ship | 0 |

**CO units.** Each belongs to one commanding officer and is normally not placed on a map.

| ID | Name | Movement type | Move |
|---|---|---|---|
| `ZCOUNIT_PARTISAN` | Partisan | Foot | 3 |
| `ZCOUNIT_RANGER` | Ranger | Mech | 3 |
| `ZCOUNIT_COMMANDO` | Commando | Mech | 4 |
| `ZCOUNIT_AT_CYCLE` | AT Cycle | Tires B | 5 |
| `ZCOUNIT_INTEL_TRUCK` | Intel Truck | Tires A | 6 |
| `ZCOUNIT_LOGIC_TRUCK` | Logistic Truck | Tires A | 5 |
| `ZCOUNIT_SMUGGLER` | Smuggler | Tires A | 8 |
| `ZCOUNIT_HOT_TANK` | Hot Tank | Tires A | 12 |
| `ZCOUNIT_AUTO_TANK` | Auto Tank | Treads | 6 |
| `ZCOUNIT_CHAPERON` | Chaperon | Treads | 6 |
| `ZCOUNIT_CRYSTAL_TANK` | Crystal Tank | Treads | 5 |
| `ZCOUNIT_REPAIR_TANK` | Repair Tank | Treads | 5 |
| `ZCOUNIT_ROYAL_GUARD` | Royal Guard | Treads | 5 |
| `ZCOUNIT_TANK_HUNTER` | Tank Hunter | Treads | 5 |
| `ZCOUNIT_SIEGE_CANNON` | Siege Cannon | Treads | 3 |
| `ZCOUNIT_IRON_SHIELD_GENERATOR` | Iron Shield Gen. | Crawler | 5 |
| `ZCOUNIT_NEOSPIDER_TANK` | Neo Spider Tank | Crawler | 5 |
| `ZCOUNIT_KIROV` | Kirov | Air | 5 |
| `ZCOUNIT_MISSILE_SUB` | Missile Submarine | Ship | 5 |

## Rules for a playable map

- Every player needs exactly one `HQ` and should have at least one `FACTORY`.
- Vehicles cannot cross rivers or mountains; infantry and mechs can. Put a `BRIDGE1` on a river hex where a road crosses.
- Bridges belong on river or sea hexes. Buildings and units belong on land.
- Place at most one building and one unit on a hex.
- Fair maps are symmetric. Two layouts that work on this grid:
  - **2 players:** mirror every position through the centre: `(x, y)` becomes `(width-1-x, height-1-y)`.
  - **3 players:** use an odd square size, measure positions from the centre hex as `(a, b)`, and rotate them:
    `(a, b)`, `(-a-b, a)`, `(b, -a-b)`.
  - **4 players:** use an odd square size, measure positions from the centre hex as `(a, b)`, and mirror them across
    both diagonals: `(a, b)`, `(b, a)`, `(-a, -b)`, `(-b, -a)`. `Hex Quad` is built this way.
    Do **not** put the four players in the four corners of the coordinate grid: two of those corners are twice as far
    from the centre as the other two.
- The short diagonal of the parallelogram runs from the top right corner to the bottom left corner. Starting positions
  near those two corners are closer together than positions in the other two corners.

## Checking a map

Start the game. Problems in a map file are written to `console.log` in the game folder as lines starting with
`Hex map import:`. A row with the wrong number of symbols, an unknown symbol, and a building or unit outside of the
map are all reported there. If the file is not valid JSON, no map is created.

## Known limits of the prototype

- Roads, rivers and coasts are drawn with the art of the square game. Roads and rivers running left to right look
  connected; diagonal ones are drawn as separate pieces. This is cosmetic and does not change movement.
- Buildings larger than one hex are not supported.

## Civilization Wars prototype

Add `"rules": ["GAMERULE_CIVILIZATION_WARS"]` to a map to play it with Food, Metal and Oil. The mode adds these
building ids, which can be used in `buildings` like any other:

| Id | Name | Each turn |
|---|---|---|
| `CIV_CITY` | City | funds, 2 Food, 2 Metal and 2 Oil |
| `CIV_WAREHOUSE` | Warehouse | funds |
| `CIV_FARMHOUSE` | Farmhouse | 1 Food for each Plains hex next to it |
| `CIV_LUMBER_MILL` | Lumber Mill | 1 Metal for each Forest hex next to it |
| `CIV_MINE` | Mine | 1 Metal, plus 1 for each Mountain hex and 4 for each iron deposit next to it |
| `CIV_OIL_REFINERY` | Oil Refinery | 1 Oil, plus 4 for each oil deposit next to it |

Resource deposits are placed on any terrain with a `deposits` list next to `buildings`; iron belongs mostly on
mountains. A deposit shows as a small icon and counts for a Mine or Oil Refinery on its hex or next to it:

```json
"deposits": [
    {"id": "IRON", "x": 13, "y": 3},
    {"id": "OIL", "x": 4, "y": 6}
]
```

A map without any deposit gets random ones when the match starts.

Every building stores its resources itself. A Mine, Lumber Mill, Farmhouse or Oil Refinery fills its own small store
(30) and stops when it is full. Only what is stored in a logistical center can be spent: a City (100 Food, 50 Metal,
50 Oil), a Warehouse (50 / 150 / 150) or a headquarters (50 / 100 / 100).

Buildings have a radius: 3 for a City, Warehouse and headquarters, 2 for the other buildings of this mode and for
a Base, Airport and Port. The hexes in the radius of a player's buildings are their territory, and its border is
always drawn in the player's colour. A center collects
the stores of your Mines, Farmhouses, Lumber Mills and Oil Refineries in its radius each turn, and a Base, Airport or
Port builds its units from the stock of the centers which have it in their radius. So place production buildings
close to a City, Warehouse or headquarters.

Cities and headquarters have a population (3 at the start, 10 at most). It eats 1 Food each per turn from the store of
its building and grows when enough Food is left. Each unit a human player builds takes 1 population from the nearest
City or headquarters.

The Truck (`CIV_TRUCK`, built at a Base) carries up to 40 resources between your buildings with Load Resources and
Unload Resources. With Auto Deliver it drives on its own when you end your turn. Each human player starts with two.

Metal is the ammunition and Oil the fuel of the units: producing or resupplying a unit takes 1 Metal for each point
of ammo and 1 Oil for each point of fuel from the stock of its owner. Machine guns and soldiers on foot are free.

A hex with a building or a road on it doesn't count as Plains anymore. All values, the build costs and the starting
resources are in `resources/scripts/general/civwars.js`. `maps/2_player/Civ Test` is the test map.
