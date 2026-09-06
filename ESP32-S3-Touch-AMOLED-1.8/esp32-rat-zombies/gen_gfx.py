#!/usr/bin/env python3
# Generator zomb_gfx.h z balicku grafika-v3/ (PNG -> 8bitove indexovane
# bitmapy do palety hry, tabulka rozmeru a kotev, animacni sekvence,
# polohy dilu zombiku pro rozpad). Pouziti:
#   python3 gen_gfx.py grafika-v3 zomb_gfx.h
import zlib, struct, json, sys, os

PACK = sys.argv[1]
OUT = sys.argv[2]

def read_png(path):
    d = open(path, 'rb').read(); assert d[:8] == b'\x89PNG\r\n\x1a\n'
    pos = 8; idat = b''; pal = None; trns = None
    while pos < len(d):
        ln, = struct.unpack('>I', d[pos:pos+4]); typ = d[pos+4:pos+8]; body = d[pos+8:pos+8+ln]; pos += 12 + ln
        if typ == b'IHDR': w, h, bd, ct, _, _, il = struct.unpack('>IIBBBBB', body); assert bd == 8 and il == 0, (path, bd, il)
        elif typ == b'PLTE': pal = [tuple(body[i:i+3]) for i in range(0, len(body), 3)]
        elif typ == b'tRNS': trns = body
        elif typ == b'IDAT': idat += body
    raw = zlib.decompress(idat)
    bpp = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]; stride = w * bpp
    out = []; prev = bytearray(stride); p = 0
    for y in range(h):
        f = raw[p]; p += 1; line = bytearray(raw[p:p+stride]); p += stride
        for i in range(stride):
            a = line[i-bpp] if i >= bpp else 0; b = prev[i]; c = prev[i-bpp] if i >= bpp else 0
            if f == 1: line[i] = (line[i] + a) & 255
            elif f == 2: line[i] = (line[i] + b) & 255
            elif f == 3: line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                pa = abs(b - c); pb = abs(a - c); pc = abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 255
        row = []
        for x in range(w):
            px = line[x*bpp:(x+1)*bpp]
            if ct == 6: r, g, b, a = px
            elif ct == 2: r, g, b = px; a = 255
            elif ct == 3: r, g, b = pal[px[0]]; a = trns[px[0]] if trns and px[0] < len(trns) else 255
            elif ct == 0: r = g = b = px[0]; a = 255
            else: r, g, b, a = px[0], px[0], px[0], px[1]
            row.append((r, g, b, a))
        out.append(row); prev = line
    return w, h, out

def hexrgb(s): s = s.lstrip('#'); return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))

# paleta: 0 pruhledna, pak paleta balicku, pak barvy hry (nebe, pas pod domy)
pal = json.load(open(os.path.join(PACK, 'palette.json')))
names = ['transp'] + list(pal.keys())
colors = [(0, 0, 0)] + [hexrgb(v) for v in pal.values()]
EXTRA = [('sky_top', '#3f7cc0'), ('sky_mid', '#5f9cd8'), ('sky_low', '#8ec0e8'), ('sky_haze', '#bcd8ee'),
         ('band', '#2b2721'), ('band_light', '#3a3429'), ('green_ok', '#5ccf5c'), ('red_bad', '#e04848')]
for n, c in EXTRA: names.append(n); colors.append(hexrgb(c))
assert len(names) <= 256

cache = {}
def color_index(rgb):
    if rgb in cache: return cache[rgb]
    best = 1; bd = 1e9
    for i in range(1, len(colors)):
        dd = sum(abs(a - b) for a, b in zip(rgb, colors[i]))
        if dd < bd: bd = dd; best = i
    if bd > 30: print('  varovani: barva', rgb, 'daleko od palety (', bd, ')')
    cache[rgb] = best
    return best

atlas = json.load(open(os.path.join(PACK, 'atlas.json')))
anims = json.load(open(os.path.join(PACK, 'animations.json')))
rigs = json.load(open(os.path.join(PACK, 'rigs.json')))

def cname(n): return 'G_' + n.upper()

lines = ['#pragma once', '', '#include <Arduino.h>', '',
 '// ===================================================================',
 '// Grafika hry Krysy a zombici - vygenerovano skriptem gen_gfx.py',
 '// z balicku grafika-v3/ (krysy-zombici-amoled-v3). Neupravovat rucne.',
 '// Bitmapy: 1 bajt na bod = index palety, 0 = pruhledna. Kotva (ax, ay)',
 '// = bod spritu, ktery se klade na svetovou pozici.',
 '// ===================================================================', '',
 'struct Bmp { const uint8_t *px; uint8_t w, h; int8_t ax, ay; };', '',
 'enum : uint8_t {']
lines.append('  ' + ', '.join('C_' + n.upper() for n in names) + ',')
lines.append('  C_COUNT')
lines.append('};')
lines.append('static const uint8_t PALETTE[C_COUNT][3] = {')
lines.append('  ' + ', '.join('{%d, %d, %d}' % c for c in colors))
lines.append('};')
lines.append('')

total = 0
for name, e in atlas.items():
    w, h, px = read_png(os.path.join(PACK, e['file']))
    assert (w, h) == (e['w'], e['h']), name
    data = []
    for row in px:
        for r, g, b, a in row:
            data.append(0 if a < 128 else color_index((r, g, b)))
    total += len(data)
    lines.append('static const uint8_t bm_%s[%d] = {' % (name, len(data)))
    for i in range(0, len(data), w):
        lines.append('  ' + ','.join(str(v) for v in data[i:i+w]) + ',')
    lines.append('};')
    ax, ay = e['anchor']['x'], e['anchor']['y']
    lines.append('static const Bmp %s = { bm_%s, %d, %d, %d, %d };   // %s' % (cname(name), name, w, h, ax, ay, e['file']))
    lines.append('')

# animace
def seq(nm, key):
    fr = anims[key]['frames']
    lines.append('static const Bmp *const %s[%d] = { %s };   // %s fps %s' % (nm, len(fr), ', '.join('&' + cname(f) for f in fr), key, anims[key].get('fps')))

ZOMBIES = ['zombie_thin_stripes', 'zombie_thin_mechanic', 'zombie_thin_punk',
           'zombie_block_worker', 'zombie_block_denim', 'zombie_block_sport',
           'zombie_round_shirt', 'zombie_round_janitor', 'zombie_round_armor']
PARTS = ['leg_back', 'arm_back', 'torso', 'leg_front', 'arm_front', 'head']   # poradi kresleni
lines.append('#define ZOMBIE_KINDS %d' % len(ZOMBIES))
lines.append('#define ZOMBIE_PARTS 6')
lines.append('#define ZOMBIE_FRAMES 6')
lines.append('static const Bmp *const ZOMBIE_WALK[ZOMBIE_KINDS][ZOMBIE_FRAMES] = {')
for z in ZOMBIES:
    lines.append('  { %s },' % ', '.join('&' + cname(f) for f in anims[z + '_walk']['frames']))
lines.append('};')
lines.append('static const uint8_t ZOMBIE_FPS[ZOMBIE_KINDS] = { %s };' % ', '.join(str(anims[z + '_walk']['fps']) for z in ZOMBIES))
lines.append('// dily v poradi kresleni: leg_back, arm_back, torso, leg_front, arm_front, head')
lines.append('static const Bmp *const ZOMBIE_PART[ZOMBIE_KINDS][ZOMBIE_PARTS] = {')
for z in ZOMBIES:
    lines.append('  { %s },' % ', '.join('&' + cname(z + '_' + p) for p in PARTS))
lines.append('};')
lines.append('// poloha leveho horniho rohu dilu v platnu snimku chuze [druh][snimek][dil][x,y]')
lines.append('static const int8_t ZOMBIE_PART_POS[ZOMBIE_KINDS][ZOMBIE_FRAMES][ZOMBIE_PARTS][2] = {')
for z in ZOMBIES:
    rows = []
    for f in anims[z + '_walk']['frames']:
        po = atlas[f]['part_offsets']
        rows.append('{ %s }' % ', '.join('{%d,%d}' % (po[p]['x'], po[p]['y']) for p in PARTS))
    lines.append('  { %s },' % ', '.join(rows))
lines.append('};')
lines.append('')
RATS = ['rat_furry', 'rat_wood', 'rat_steel']
lines.append('#define RAT_KINDS 3')
lines.append('#define RAT_FRAMES 6')
lines.append('// [krysa][0 bezna / 1 s bourakem][snimek]')
lines.append('static const Bmp *const RAT_DRIVE[RAT_KINDS][2][RAT_FRAMES] = {')
for r in RATS:
    lines.append('  { { %s },' % ', '.join('&' + cname(f) for f in anims[r + '_drive']['frames']))
    lines.append('    { %s } },' % ', '.join('&' + cname(f) for f in anims[r + '_armed_drive']['frames']))
lines.append('};')
lines.append('static const Bmp *const RAT_STILL[RAT_KINDS] = { %s };' % ', '.join('&' + cname(r) for r in RATS))
seq('PIGEON_FLY', 'pigeon_fly')
seq('IMPACT', 'impact')
seq('DUST', 'dust')
seq('SWING', 'wrecking_swing')
lines.append('static const int8_t SWING_BALL[%d][2] = { %s };   // stred zavazi ve snimku' % (len(anims['wrecking_swing']['frames']),
             ', '.join('{%d,%d}' % (atlas[f]['ball_center']['x'], atlas[f]['ball_center']['y']) for f in anims['wrecking_swing']['frames'])))
lines.append('static const Bmp *const FUEL_GAUGE[11] = { %s };' % ', '.join('&G_FUEL_GAUGE_%d' % i for i in range(11)))
lines.append('static const Bmp *const CLOUDS[2] = { &G_CLOUD_0, &G_CLOUD_1 };')
lines.append('static const Bmp *const GARAGES[3] = { &G_GARAGE_RED, &G_GARAGE_TEAL, &G_GARAGE_YELLOW };')
lines.append('#define BUILDING_BG_COUNT 8')
lines.append('static const Bmp *const BUILDINGS_BG[BUILDING_BG_COUNT] = { &G_BUILDING_GREEN_BG, &G_BUILDING_RED_BG, &G_FACTORY_BG, &G_SHACK_BG, &G_SHACK_SMALL_BG, &G_HALFPIPE_BG, &G_FENCE_BG, &G_CITY_FAR_BG };   // tlumene, vrstva 0.5')
lines.append('#define NEAR_PROP_COUNT 14')
lines.append('static const Bmp *const NEAR_PROPS[NEAR_PROP_COUNT] = { &G_FENCE, &G_TRASH_BIN, &G_TIRE_STACK, &G_STREETLAMP, &G_BARREL, &G_RUBBISH, &G_BUSH, &G_STONES, &G_ROAD_SIGN, &G_CONE, &G_WEEDS, &G_PALLETS, &G_CONTAINER, &G_HALFPIPE };   // plne barevne, vrstva 1.0')
lines.append('static const Bmp *const BYSTANDERS[7] = { &G_PERSON_WORKER, &G_PERSON_WOMAN, &G_PERSON_YOUTH, &G_DOG_BROWN, &G_DOG_SPOTTED, &G_CAT_ORANGE, &G_CAT_GRAY };   // plne barevne, vrstva 1.0')
lines.append('')
open(OUT, 'w').write('\n'.join(lines) + '\n')
print('sprites:', len(atlas), 'bajtu bitmap:', total, 'barev palety:', len(names))
