import zlib, struct, json, re, sys, os
PACK=sys.argv[1]; OUT=sys.argv[2]; PALH=sys.argv[3]

def read_png(path):
    d=open(path,'rb').read(); assert d[:8]==b'\x89PNG\r\n\x1a\n'
    pos=8; idat=b''; pal=None; trns=None
    while pos<len(d):
        ln,=struct.unpack('>I',d[pos:pos+4]); typ=d[pos+4:pos+8]; body=d[pos+8:pos+8+ln]; pos+=12+ln
        if typ==b'IHDR': w,h,bd,ct,_,_,il=struct.unpack('>IIBBBBB',body); assert bd==8 and il==0, (bd,il)
        elif typ==b'PLTE': pal=[tuple(body[i:i+3]) for i in range(0,len(body),3)]
        elif typ==b'tRNS': trns=body
        elif typ==b'IDAT': idat+=body
    raw=zlib.decompress(idat)
    bpp={0:1,2:3,3:1,4:2,6:4}[ct]; stride=w*bpp
    out=[]; prev=bytearray(stride); p=0
    for y in range(h):
        f=raw[p]; p+=1; line=bytearray(raw[p:p+stride]); p+=stride
        for i in range(stride):
            a=line[i-bpp] if i>=bpp else 0; b=prev[i]; c=prev[i-bpp] if i>=bpp else 0
            if f==1: line[i]=(line[i]+a)&255
            elif f==2: line[i]=(line[i]+b)&255
            elif f==3: line[i]=(line[i]+(a+b)//2)&255
            elif f==4:
                pa=abs(b-c); pb=abs(a-c); pc=abs(a+b-2*c)
                pr=a if pa<=pb and pa<=pc else (b if pb<=pc else c)
                line[i]=(line[i]+pr)&255
        row=[]
        for x in range(w):
            px=line[x*bpp:(x+1)*bpp]
            if ct==6: r,g,b,a=px
            elif ct==2: r,g,b=px; a=255
            elif ct==3: r,g,b=pal[px[0]]; a=trns[px[0]] if trns and px[0]<len(trns) else 255
            elif ct==0: r=g=b=px[0]; a=255
            else: r,g,b,a=px[0],px[0],px[0],px[1]
            row.append((r,g,b,a))
        out.append(row); prev=line
    return w,h,out

# existing palette from rat_palette.h
pal=open(PALH,encoding='utf-8').read()
enum=re.findall(r'C_\w+', pal.split('enum : uint8_t {')[1].split('C_COUNT')[0])
rows=re.findall(r'\{(\d+), (\d+), (\d+)\}', pal.split('PALETTE[C_COUNT][3] = {')[1].split('};')[0])
base=[tuple(map(int,r)) for r in rows]
extra=[]   # new colors beyond C_COUNT
def color_index(rgb):
    best=None;bd=1e9
    for i,c in enumerate(base+extra):
        dd=sum(abs(a-b) for a,b in zip(rgb,c))
        if dd<bd: bd=dd;best=i
    if bd<=18 and best!=0: return best
    extra.append(rgb); return len(base)+len(extra)-1

pool=[c for c in "kKgGlpwryoOLmMvVtsSRebcCnNxXH0123456789ABDFIJPQTUWYZadfhijqu!#$%&()*+,-/:;<=>?@[]^_`{|}~"]
chars={}   # idx -> char
def ch_for(idx):
    if idx not in chars:
        chars[idx]=pool.pop(0)
    return chars[idx]

def to_rows(path):
    w,h,px=read_png(path)
    out=[]
    for row in px:
        s=''
        for r,g,b,a in row:
            s+='.' if a<128 else ch_for(color_index((r,g,b)))
        out.append(s)
    return w,h,out

sprites=[('SPR_RAT_RUN0','png/rat_run_0.png'),('SPR_RAT_RUN1','png/rat_run_1.png'),('SPR_RAT_JUMP','png/rat_jump.png'),
 ('SPR_SPIDER0','png/spider_0.png'),('SPR_SPIDER1','png/spider_1.png'),('SPR_CAN','png/can.png'),('SPR_PEAR','png/pear.png'),
 ('SPR_PAPER','png/paper.png'),('SPR_ROLL','png/toilet_roll.png'),('SPR_PIPE_STUB','png/pipe.png'),('SPR_CRATE','png/crate.png'),
 ('SPR_SLIME','png/slime.png'),('SPR_HEART','png/heart_full.png'),('SPR_HEART_EMPTY','png/heart_empty.png'),
 ('TILE_BRICK0','tiles/brick_0.png'),('TILE_BRICK1','tiles/brick_1.png'),('TILE_BRICK2','tiles/brick_2.png'),
 ('TILE_CEILING','tiles/ceiling_repeat.png'),('TILE_WALKWAY','tiles/walkway_repeat.png'),
 ('TILE_HOLE_EDGE_L','tiles/hole_edge_left.png'),('TILE_HOLE_EDGE_R','tiles/hole_edge_right.png'),
 ('TILE_WATER0','tiles/water_0.png'),('TILE_WATER1','tiles/water_1.png'),('TILE_WATER2','tiles/water_2.png'),('TILE_WATER3','tiles/water_3.png'),
 ('TILE_HOLE_WATER0','tiles/hole_water_0.png'),('TILE_HOLE_WATER1','tiles/hole_water_1.png'),('TILE_HOLE_WATER2','tiles/hole_water_2.png'),('TILE_HOLE_WATER3','tiles/hole_water_3.png'),
 ('TILE_SHELF','tiles/shelf_repeat.png'),('TILE_SHELF_CAP_L','tiles/shelf_cap_left.png'),('TILE_SHELF_CAP_R','tiles/shelf_cap_right.png'),('TILE_MOSS','tiles/moss.png'),
 ('TILE_PANEL_INTRO','tiles/panel_intro.png'),('TILE_PANEL_OVER','tiles/panel_game_over.png'),('TILE_THREAD','tiles/spider_thread.png'),
 ('TILE_SPLASH0','tiles/splash_0.png'),('TILE_SPLASH1','tiles/splash_1.png'),('TILE_SPLASH2','tiles/splash_2.png'),('TILE_SPLASH3','tiles/splash_3.png'),
 ('TILE_SPARK0','tiles/pickup_spark_0.png'),('TILE_SPARK1','tiles/pickup_spark_1.png'),('TILE_SPARK2','tiles/pickup_spark_2.png'),('TILE_SPARK3','tiles/pickup_spark_3.png')]
body=[]
for name,f in sprites:
    w,h,rs=to_rows(os.path.join(PACK,f))
    body.append(f"// {f} {w}x{h}\nSPRITE({name},\n" + ",\n".join('  "'+r+'"' for r in rs) + ")\n")
hdr=['#pragma once','','#include <Arduino.h>','#include "rat_sprites_def.h"','',
'// ===================================================================',
'// Sada spritu ASTRA - vygenerovano skriptem z balicku kanal-komplet.zip',
'// (ChatGPT Astra). Neupravovat rucne; znaky -> barvy podle ASTRA_LEGEND',
'// (barvy nad C_COUNT jsou v PALETTE_EXTRA). Obsahuje i dlazdice prostredi.',
'// ===================================================================','',
'#define SPRITES_HAVE_TILES 1','',
f'#define PALETTE_EXTRA_COUNT {len(extra)}',
'static const uint8_t PALETTE_EXTRA[PALETTE_EXTRA_COUNT > 0 ? PALETTE_EXTRA_COUNT : 1][3] = {',
'  ' + ', '.join('{%d, %d, %d}'%c for c in extra) + ('' if extra else '{0, 0, 0}'),
'};','',
'// znak -> index barvy (C_* nebo C_COUNT + k)',
'struct LegendEntry { char ch; uint8_t idx; };',
'static const LegendEntry ASTRA_LEGEND[] = {']
for idx,ch in sorted(chars.items(), key=lambda kv: kv[1]):
    nm = enum[idx] if idx < len(base) else f'C_COUNT + {idx-len(base)}'
    esc = "'\\\\'" if ch=='\\' else ("'\\''" if ch=="'" else f"'{ch}'")
    hdr.append(f'  {{ {esc}, {nm} }},')
hdr.append('};'); hdr.append('#define ASTRA_LEGEND_COUNT ((int)(sizeof(ASTRA_LEGEND) / sizeof(ASTRA_LEGEND[0])))'); hdr.append('')
open(OUT,'w',encoding='utf-8').write("\n".join(hdr)+"\n"+"\n".join(body))
print("sprites:",len(sprites),"colors used:",len(chars),"extra colors:",len(extra), extra)
