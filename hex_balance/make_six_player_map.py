#!/usr/bin/env python3
"""Creates the six player hex map "Hex Crown" as maps/6_player/Hex Crown.hexmap.json.

The board is a hexagon with a radius of 20 fields. One sixth of it is described below and rotated five times,
so all six players have the same surroundings.

Coordinates of the described sixth: a field is (i, j) with i >= 1 and j >= 0. It lies i steps from the center
towards one corner of the hexagon and j steps towards the next corner, so its distance to the center is i + j.
i == j is the middle of the sixth (the middle of an edge of the hexagon), j == 0 is the border to the next player.
"""
import json
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RADIUS = 20
SIZE = 2 * RADIUS + 1
PLAYERS = 6

# ---------------------------------------------------------------------------------------------- one sixth of the map
SEA_FROM = 19            # outer sea ring, connects the harbours of all players
COAST = 18
MOAT = (4, 5)            # sea ring around the central island
RING_ROAD = 10

terrain = {}             # (i, j) -> symbol
buildings = []           # (id, i, j, owned)
units = []               # (id, i, j)

def put(symbol, *fields):
    for field in fields:
        terrain[field] = symbol

for d in range(1, RADIUS + 1):
    for j in range(0, d):
        i = d - j
        if d >= SEA_FROM or d in MOAT:
            terrain[(i, j)] = "S"
        elif d == RING_ROAD:
            terrain[(i, j)] = "="
        elif d == COAST and 3 <= abs(j - i) <= 6:
            terrain[(i, j)] = "BE"
        elif 6 <= d <= 17 and (i * 7 + j * 13) % 11 == 0:
            terrain[(i, j)] = "F"

# border to the next player: road from the central island to the ring road, then a mountain ridge with one pass
put("B", (4, 0), (5, 0))
put("=", (2, 0), (3, 0), (6, 0), (7, 0), (9, 0))
put("M", (13, 0), (14, 0), (16, 0), (17, 0), (13, 1), (16, 1), (1, 13), (1, 16))
put(".", (15, 0), (11, 0), (12, 0))
# home: road from the ring road through the base to the harbour
put("=", (6, 5), (6, 6), (8, 7), (8, 8), (9, 8))
put("M", (4, 7), (7, 4), (10, 6), (6, 10), (11, 3), (3, 11), (5, 2), (2, 5), (9, 2), (2, 9), (12, 5), (5, 12))
put(".", (7, 7), (7, 6), (6, 7), (7, 8), (9, 9), (8, 6), (6, 8), (5, 6), (7, 5), (5, 7), (9, 7), (1, 1))

buildings += [("HQ", 7, 7, True), ("FACTORY", 7, 6, True), ("FACTORY", 6, 7, True), ("AIRPORT", 7, 8, True),
              ("HARBOUR", 9, 9, True), ("TOWN", 8, 6, True), ("TOWN", 6, 8, True)]
# neutral buildings of the sixth
buildings += [("TOWN", 1, 1, False),                      # central island
              ("FACTORY", 8, 0, False), ("AIRPORT", 12, 0, False), ("HARBOUR", 18, 0, False),   # border to the next player
              ("TOWN", 4, 9, False), ("TOWN", 9, 4, False), ("TOWN", 3, 6, False), ("TOWN", 6, 3, False),
              ("TOWN", 10, 2, False), ("TOWN", 2, 10, False), ("TOWN", 12, 3, False), ("TOWN", 3, 12, False),
              ("TOWN", 11, 6, False), ("TOWN", 6, 11, False), ("TOWN", 15, 1, False), ("TOWN", 2, 15, False),
              ("TOWN", 4, 4, False)]
units += [("INFANTRY", 8, 7), ("INFANTRY", 7, 5), ("INFANTRY", 5, 7), ("MECH", 6, 6),
          ("LIGHT_TANK", 5, 6), ("RECON", 6, 5), ("ARTILLERY", 9, 7)]

# buildings and units replace the scattered forests
for _, i, j, _ in buildings:
    if terrain.get((i, j)) == "F":
        terrain[(i, j)] = "."
for _, i, j in units:
    if terrain.get((i, j)) == "F":
        terrain[(i, j)] = "."

# ---------------------------------------------------------------------------------------------- build the whole map

def to_axial(i, j):
    """offset from the center of the field (i, j) of the first sixth"""
    return i + j, -i

def rotate(a, b, times):
    for _ in range(times):
        a, b = -b, a + b
    return a, b

def distance(a, b):
    return (abs(a) + abs(b) + abs(a + b)) // 2

def position(i, j, player):
    a, b = rotate(*to_axial(i, j), player)
    return RADIUS + a, RADIUS + b

grid = [["." for _ in range(SIZE)] for _ in range(SIZE)]
for y in range(SIZE):
    for x in range(SIZE):
        if distance(x - RADIUS, y - RADIUS) > RADIUS:
            # no tile: the map is shown as hexagon
            grid[y][x] = "X"

problems = []
used_buildings = {}
used_units = {}
out_buildings = [{"id": "TOWER", "x": RADIUS, "y": RADIUS, "owner": 0}]
out_units = []
for player in range(PLAYERS):
    for (i, j), symbol in terrain.items():
        x, y = position(i, j, player)
        grid[y][x] = symbol
for player in range(PLAYERS):
    for building, i, j, owned in buildings:
        x, y = position(i, j, player)
        if (x, y) in used_buildings:
            problems.append("two buildings on %d,%d" % (x, y))
        if grid[y][x] in ("S", "M", "F", "B", "BE", "X"):
            problems.append("%s on terrain %s at sixth field %d,%d" % (building, grid[y][x], i, j))
        used_buildings[(x, y)] = building
        out_buildings.append({"id": building, "x": x, "y": y, "owner": player + 1 if owned else 0})
    for unit, i, j in units:
        x, y = position(i, j, player)
        if (x, y) in used_units:
            problems.append("two units on %d,%d" % (x, y))
        if grid[y][x] in ("S", "M"):
            problems.append("%s on terrain %s at sixth field %d,%d" % (unit, grid[y][x], i, j))
        used_units[(x, y)] = unit
        out_units.append({"id": unit, "x": x, "y": y, "owner": player + 1})
# harbours need the sea next to them
NEIGHBOURS = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, -1), (-1, 1)]
for entry in out_buildings:
    if entry["id"] == "HARBOUR":
        if not any(grid[entry["y"] + dy][entry["x"] + dx] == "S" for dx, dy in NEIGHBOURS
                   if 0 <= entry["x"] + dx < SIZE and 0 <= entry["y"] + dy < SIZE):
            problems.append("harbour without sea at %d,%d" % (entry["x"], entry["y"]))
if problems:
    raise SystemExit("\n".join(sorted(set(problems))))

out_buildings.sort(key=lambda b: (b["owner"] == 0, b["owner"], b["y"], b["x"]))
out_units.sort(key=lambda u: (u["owner"], u["y"], u["x"]))
rows = [" " * y + " ".join("%-2s" % symbol for symbol in grid[y]).rstrip() for y in range(SIZE)]

lines = ["{",
         '    "name": "Hex Crown",',
         '    "description": "Very large six player map on a hexagon. Every player has a harbour on the outer sea and an airport. '
         'A ring road connects the bases, mountain ridges with one pass separate the neighbours and six bridges lead to the central island.",',
         '    "author": "Hex prototype",',
         '    "players": %d,' % PLAYERS,
         '    "width": %d,' % SIZE,
         '    "height": %d,' % SIZE,
         '    "legend": {".": "PLAINS", "F": "FOREST", "M": "MOUNTAIN", "=": "STREET", "B": "BRIDGE1", "S": "SEA", "BE": "BEACH", "X": "VOID"},',
         '    "terrain": [']
lines += ["        %s%s" % (json.dumps(row), "," if index < SIZE - 1 else "") for index, row in enumerate(rows)]
lines += ["    ],", '    "buildings": [']
lines += ["        %s%s" % (json.dumps(entry), "," if index < len(out_buildings) - 1 else "") for index, entry in enumerate(out_buildings)]
lines += ["    ],", '    "units": [']
lines += ["        %s%s" % (json.dumps(entry), "," if index < len(out_units) - 1 else "") for index, entry in enumerate(out_units)]
lines += ["    ]", "}"]
path = os.path.join(REPO, "maps", "6_player", "Hex Crown.hexmap.json")
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, "w", encoding="utf-8", newline="\n") as handle:
    handle.write("\n".join(lines) + "\n")
json.load(open(path, encoding="utf-8"))

counts = {}
for entry in out_buildings:
    counts[entry["id"]] = counts.get(entry["id"], 0) + 1
land = sum(1 for row in grid for symbol in row if symbol not in ("S", "X"))
print("wrote %s: %dx%d, %d land fields, buildings %s, %d units" % (path, SIZE, SIZE, land, counts, len(out_units)))
