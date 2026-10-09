"""Creates "Hex Globe": a four player map which wraps around in east west direction.

The map is one sector repeated four times around the globe, so all players have the same position.
Run it from anywhere, the game imports the written *.hexmap.json at the next start.
"""
import json
import os

SECTORS = 4
# one sector, eight columns wide. The rows are written as rectangle: odd rows are drawn half a field to the right.
SECTOR = [
    "S S S S S S S S",
    ". . F . . . M .",
    ". . . . F . . .",
    ". . . . . . M M",
    ". . . . . F . M",
    ". . . . . . . .",
    "= = = = = = = =",
    ". . . . . . . .",
    ". F . . . . M M",
    ". . . . . . M .",
    ". . . . F . . .",
    ". F . . . . M .",
    "S S S S S S S S",
]
SECTOR_WIDTH = 8
# x, y inside of the sector, owner 0 is neutral and 1 the owner of the sector
BUILDINGS = [
    ("HQ", 3, 6, 1), ("FACTORY", 2, 5, 1), ("FACTORY", 4, 7, 1), ("TOWN", 3, 4, 1), ("TOWN", 3, 8, 1),
    ("TOWN", 6, 2, 0), ("TOWN", 7, 6, 0), ("TOWN", 6, 10, 0), ("TOWN", 0, 3, 0), ("TOWN", 0, 9, 0),
    ("AIRPORT", 1, 7, 0),
]
UNITS = [
    ("INFANTRY", 2, 6), ("INFANTRY", 4, 6), ("INFANTRY", 3, 7), ("LIGHT_TANK", 3, 5), ("RECON", 2, 7),
]


def main():
    rows = []
    for y, row in enumerate(SECTOR):
        cells = row.split()
        assert len(cells) == SECTOR_WIDTH, y
        rows.append(("" if y % 2 == 0 else " ") + " ".join(cells * SECTORS))
    buildings = []
    units = []
    for sector in range(SECTORS):
        for building, x, y, owner in BUILDINGS:
            buildings.append({"id": building, "x": x + sector * SECTOR_WIDTH, "y": y, "owner": (sector + 1) * owner})
        for unit, x, y in UNITS:
            units.append({"id": unit, "x": x + sector * SECTOR_WIDTH, "y": y, "owner": sector + 1})
    data = {
        "name": "Hex Globe",
        "description": "A world which wraps around: leaving the map in the east arrives in the west. "
                       "A road follows the equator around the globe.",
        "author": "Hex prototype",
        "players": SECTORS,
        "width": SECTOR_WIDTH * SECTORS,
        "height": len(SECTOR),
        "wrap": True,
        "legend": {".": "PLAINS", "F": "FOREST", "M": "MOUNTAIN", "=": "STREET", "S": "SEA"},
        "terrain": rows,
        "buildings": buildings,
        "units": units,
    }
    target = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "maps", "4_player", "Hex Globe.hexmap.json")
    with open(target, "w", encoding="utf-8") as stream:
        json.dump(data, stream, indent=4)
    print("written", os.path.normpath(target))


if __name__ == "__main__":
    main()
