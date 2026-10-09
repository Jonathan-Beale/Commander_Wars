Earth Hex Compiler for the Commander Wars hex prototype

Generated maps:
  Earth_180x90.hexmap.json
  Earth_360x180.hexmap.json
  Earth_540x270.hexmap.json
  Earth_720x360.hexmap.json
  Earth_1080x540.hexmap.json

Usage:
  python earth_hex_compiler.py --all --out earth_maps
  python earth_hex_compiler.py --size 360x180 --out earth_maps
  python earth_hex_compiler.py --size 360x180 --size 720x360 --out earth_maps
  python earth_hex_compiler.py --all --terrain-only --out earth_maps

Implementation:
- East/west wrapping world maps.
- Coastlines and inland-water masking come from Basemap's bundled GSHHS data, so generation works offline.
- Broad gameplay-oriented climate and mountain classification is procedural/generalized.
- Major rivers are generalized polylines and appear at 360 columns and above.
- Four scenario starts are included so maps satisfy the prototype's HQ/factory requirement.
- These starts are deliberately NOT competitively balanced.
- Re-run at arbitrary resolutions with --size WIDTHxHEIGHT.

Install requirements if needed:
  pip install numpy matplotlib basemap
