#!/usr/bin/env python3
"""Earth -> Commander Wars hexmap compiler.

Uses Basemap's bundled GSHHS coast/lake polygons so it can generate maps offline.
The terrain classifier adds broad climate/mountain regions for gameplay.  The
output uses the prototype's east-west wrapping rectangular hex layout.

Examples:
  python earth_hex_compiler.py --all --out maps
  python earth_hex_compiler.py --size 360x180 --out maps
  python earth_hex_compiler.py --size 720x360 --terrain-only --out maps
"""
from __future__ import annotations
import argparse, json, math
from pathlib import Path
import numpy as np
from mpl_toolkits.basemap import maskoceans

DEFAULT_SCALES=[(180,90),(360,180),(540,270),(720,360),(1080,540)]
LEGEND={
    '.':'PLAINS','F':'FOREST','M':'MOUNTAIN','S':'SEA','~':'RIVER',
    'D':'DESERT','d':'DESERT_ROCK','N':'SNOW','n':'SNOW_MOUNTAIN',
    'W':'WASTELAND'
}
# Major mountain chains as lon/lat segments. Broad, deliberately generalized.
MOUNTAIN_SEGMENTS=[
    # Andes
    ((-78,-5),(-70,-55)),((-80,8),(-76,-5)),
    # Rockies / Alaska
    ((-123,55),(-106,30)),((-150,63),(-123,50)),
    # Appalachians
    ((-84,34),(-70,47)),
    # Alps / Carpathians / Caucasus
    ((5,45),(17,48)),((17,48),(27,45)),((39,42),(48,43)),
    # Atlas
    ((-10,31),(10,36)),
    # Himalaya / Hindu Kush / Tian Shan
    ((68,35),(95,29)),((60,37),(78,42)),((72,42),(94,43)),
    # East African highlands
    ((35,12),(38,-5)),
    # Zagros
    ((44,37),(56,27)),
    # Japan / Taiwan
    ((130,32),(143,44)),((121,22),(121,25)),
    # New Guinea / New Zealand
    ((135,-5),(149,-7)),((166,-45),(177,-36)),
    # Great Dividing Range
    ((146,-18),(150,-38)),
    # Scandinavia
    ((6,58),(20,70)),
]
# Important rivers as generalized lon/lat polylines. Rasterized only when the
# map is large enough to make a one-hex river useful.
RIVERS=[
    [(-92,47),(-90,43),(-91,38),(-90,34),(-91,30)], # Mississippi
    [(-75,-5),(-65,-4),(-55,-2),(-50,-1)],          # Amazon
    [(31,-1),(31,10),(31,20),(31,30)],              # Nile
    [(90,31),(100,30),(110,30),(121,31)],           # Yangtze
    [(96,34),(105,35),(113,34),(119,37)],           # Yellow
    [(24,-12),(18,-6),(15,-5),(12,-6)],             # Congo-ish
    [(9,48),(18,47),(25,45),(29,45)],               # Danube
    [(67,30),(76,25),(86,24),(90,23)],              # Ganges/Brahmaputra
]

# Four non-balanced world-scenario starts; nearest land cell is used.
STARTS=[
    ('North America',-77.0,38.9),
    ('South America',-47.9,-15.8),
    ('Europe',10.0,50.0),
    ('East Asia',116.4,39.9),
]

# Neutral city/property anchors. Type chosen for gameplay, not literal building identity.
CITIES=[
    (-74.0,40.7),(-118.2,34.1),(-99.1,19.4),(-46.6,-23.6),(-58.4,-34.6),
    (-0.1,51.5),(2.35,48.86),(13.4,52.5),(12.5,41.9),(30.5,50.45),(37.6,55.75),
    (31.2,30.0),(3.4,6.5),(28.0,-26.2),(36.8,-1.3),
    (72.9,19.1),(77.2,28.6),(90.4,23.8),(100.5,13.8),(103.8,1.35),
    (106.8,-6.2),(116.4,39.9),(121.5,31.2),(114.2,22.3),(139.7,35.7),(126.98,37.56),
    (151.2,-33.9),(144.96,-37.8),
]


def lonlat_grid(w,h):
    # Cell centers, truncating Antarctica to keep useful world area.
    lons=np.linspace(-180+90/w,180-90/w,w)
    lats=np.linspace(82-80/h,-78+80/h,h)
    return np.meshgrid(lons,lats)


def land_mask(lon2,lat2):
    dummy=np.zeros_like(lon2)
    # maskoceans masks ocean and inland lakes using bundled GSHHS polygons.
    m=maskoceans(lon2,lat2,dummy,inlands=True,resolution='l',grid=1.25)
    return ~np.ma.getmaskarray(m)


def point_seg_dist(lon,lat,a,b):
    # Approximate equirectangular degrees, longitude corrected by cos(latitude).
    x=lon*math.cos(math.radians(lat)); y=lat
    ax=a[0]*math.cos(math.radians(lat)); ay=a[1]
    bx=b[0]*math.cos(math.radians(lat)); by=b[1]
    vx,vy=bx-ax,by-ay; wx,wy=x-ax,y-ay
    vv=vx*vx+vy*vy
    t=0 if vv==0 else max(0,min(1,(wx*vx+wy*vy)/vv))
    return math.hypot(x-(ax+t*vx),y-(ay+t*vy))


def mountain(lon,lat,res_factor):
    width=max(0.7,2.3/res_factor)
    return any(point_seg_dist(lon,lat,a,b)<width for a,b in MOUNTAIN_SEGMENTS)


def terrain_for(lon,lat,res_factor):
    alat=abs(lat)
    # Polar / subpolar
    if alat>68: return 'N'
    # Major deserts, broad geographic boxes.
    desert=(
        (-17<=lon<=35 and 15<=lat<=33) or      # Sahara
        (35<=lon<=62 and 15<=lat<=32) or       # Arabia
        (110<=lon<=145 and -34<=lat<=-18) or   # Australia interior
        (-78<=lon<=-68 and -30<=lat<=-15) or   # Atacama
        (12<=lon<=28 and -30<=lat<=-17) or     # Namib/Kalahari
        (75<=lon<=110 and 35<=lat<=48) or      # Central Asia/Gobi
        (-120<=lon<=-103 and 28<=lat<=40)      # SW North America
    )
    is_m=mountain(lon,lat,res_factor)
    if is_m: return 'd' if desert else ('n' if alat>58 else 'M')
    if desert: return 'D'
    # Tropical forests: Amazon, Congo, SE Asia + equatorial wet belt.
    rainforest=(
        (-80<=lon<=-48 and -15<=lat<=8) or
        (10<=lon<=32 and -8<=lat<=8) or
        (95<=lon<=155 and -12<=lat<=15)
    )
    if rainforest: return 'F'
    # Boreal forests and temperate wet regions.
    if 48<=lat<=66 and ((-135<=lon<=-55) or (15<=lon<=150)): return 'F'
    if -50<=lat<=-35 and (140<=lon<=180): return 'F'
    # Some forests elsewhere, deterministic spatial texture.
    moisture=math.sin(math.radians(lon*2.7))+0.65*math.cos(math.radians(lat*5.1))
    if alat<58 and moisture>1.05: return 'F'
    if alat>58: return 'W'
    return '.'


def nearest_land(grid,lon,lat,w,h,maxr=20):
    x=int(((lon+180)/360)*w)%w
    y=int(((82-lat)/160)*h)
    y=max(0,min(h-1,y))
    if grid[y][x] not in ('S','~','M','d','n'): return x,y
    for r in range(1,maxr+1):
        for dy in range(-r,r+1):
            for dx in (-r,r):
                xx=(x+dx)%w; yy=y+dy
                if 0<=yy<h and grid[yy][xx] not in ('S','~','M','d','n'): return xx,yy
        for dx in range(-r+1,r):
            for dy in (-r,r):
                xx=(x+dx)%w; yy=y+dy
                if 0<=yy<h and grid[yy][xx] not in ('S','~','M','d','n'): return xx,yy
    return None


def draw_rivers(grid,w,h):
    if w<300: return
    def cell(lon,lat):
        return int(((lon+180)/360)*w)%w, max(0,min(h-1,int(((82-lat)/160)*h)))
    for pts in RIVERS:
        for (lon1,lat1),(lon2,lat2) in zip(pts,pts[1:]):
            x1,y1=cell(lon1,lat1); x2,y2=cell(lon2,lat2)
            dx=x2-x1
            if abs(dx)>w//2: dx-=int(math.copysign(w,dx))
            n=max(abs(dx),abs(y2-y1),1)
            for i in range(n+1):
                x=round(x1+dx*i/n)%w; y=round(y1+(y2-y1)*i/n)
                if 0<=y<h and grid[y][x] not in ('S','M','d','n'):
                    grid[y][x]='~'


def build(w,h,terrain_only=False):
    lon2,lat2=lonlat_grid(w,h)
    land=land_mask(lon2,lat2)
    grid=[['S']*w for _ in range(h)]
    res_factor=w/360
    for y in range(h):
        for x in range(w):
            if land[y,x]: grid[y][x]=terrain_for(float(lon2[y,x]),float(lat2[y,x]),res_factor)
    draw_rivers(grid,w,h)
    buildings=[]
    if not terrain_only:
        occupied=set()
        for owner,(_,lon,lat) in enumerate(STARTS,1):
            p=nearest_land(grid,lon,lat,w,h)
            if not p: continue
            x,y=p; buildings.append({'id':'HQ','x':x,'y':y,'owner':owner}); occupied.add((x,y))
            # nearest nearby buildable land for factory; search outward so every start gets one.
            placed=False
            for r in range(1,8):
                for dy in range(-r,r+1):
                    for dx in range(-r,r+1):
                        if max(abs(dx),abs(dy)) != r: continue
                        xx=(x+dx)%w; yy=y+dy
                        if 0<=yy<h and grid[yy][xx] not in ('S','~','M','d','n') and (xx,yy) not in occupied:
                            buildings.append({'id':'FACTORY','x':xx,'y':yy,'owner':owner}); occupied.add((xx,yy)); placed=True; break
                    if placed: break
                if placed: break
        # Neutral towns only if they do not collide with starts.
        for lon,lat in CITIES:
            p=nearest_land(grid,lon,lat,w,h,maxr=5)
            if p and p not in occupied:
                x,y=p; buildings.append({'id':'TOWN','x':x,'y':y,'owner':0}); occupied.add(p)
    return {
        'name':f'Earth {w}x{h}',
        'description':('Earth-scale east-west wrapping map generated from GSHHS coastlines with broad climate, '
                       'mountain, river and city abstractions. Four world-scenario starts; not competitively balanced.'),
        'author':'OpenAI Earth Hex Compiler',
        'players':4,'width':w,'height':h,'wrap':True,
        'legend':LEGEND,
        'terrain':[' '.join(r) for r in grid],
        'buildings':buildings,'units':[]
    }


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--size',action='append',help='WIDTHxHEIGHT; may be repeated')
    ap.add_argument('--all',action='store_true',help='Generate the standard five scales')
    ap.add_argument('--out',default='earth_maps')
    ap.add_argument('--terrain-only',action='store_true')
    args=ap.parse_args()
    sizes=DEFAULT_SCALES if args.all or not args.size else []
    if args.size:
        sizes += [tuple(map(int,s.lower().split('x'))) for s in args.size]
    out=Path(args.out); out.mkdir(parents=True,exist_ok=True)
    for w,h in sizes:
        print(f'Generating {w}x{h}...')
        data=build(w,h,args.terrain_only)
        p=out/f'Earth_{w}x{h}.hexmap.json'
        p.write_text(json.dumps(data,separators=(',',':')),encoding='utf-8')
        print(f'  {p} ({p.stat().st_size/1024/1024:.1f} MiB)')

if __name__=='__main__': main()
