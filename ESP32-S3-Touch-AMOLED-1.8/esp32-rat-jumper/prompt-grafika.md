# Krysa skokan – kompletní zadání včetně grafiky

Toto je souhrn, ze kterého se dá hra znovu vytvořit nebo předat grafikovi. Kód: `ESP32-S3-Touch-AMOLED-1.8/esp32-rat-jumper/`, specifikace `rat-jumper.md`.

## Zadání hry

Dětská skákačka (endless runner) pro Waveshare ESP32-S3-Touch-AMOLED-1.8 (AMOLED 368×448 px, dotyk, tlačítko BOOT, reproduktor). Displej se drží **na šířku, BOOT dole**. Krysa běží zleva doprava kanálem (stokou), hráč skáče přes překážky, po policích z trubek se dostává do vyšších pater a sbírá odpadky k jídlu. Grafika dětská, barevná, ve stylu 16bitových skákaček (Prehistorik): syté barvy, tmavé obrysy, jednoduché tvary. Obraz prochází filtrem starého barevného CRT monitoru (scanlines, RGB maska, měkké hrany, vinětace, blikání).

- Ovládání: ťuknutí kamkoli = skok; BOOT = vysoký skok; v úvodu a po konci ťuknutí = nová hra.
- Pravidla: 3 srdíčka; náraz do překážky nebo pavouka nebo pád do vody ubere srdíčko a krysa je 1,6 s nezranitelná (bliká); bez srdíček KONEC s body a nejlepším výsledkem. Každý sebraný odpadek = 1 bod. Svět zrychluje z 44 na 96 bodů/s (o 1,2 za s).
- Zvuky (bez hudby): skok, vysoký skok, dopad, sebrání, zásah, šplouchnutí, konec, start; „8bitové" obdélníkové tóny a šum.

## Herní plocha

Logické rozlišení **150 × 123 bodů**, 1 bod = 3×3 px displeje. Souřadnice y roste dolů.

| Prvek | Umístění (body) |
|---|---|
| strop | y 0–8 (výška 9), tmavé kameny, krápníky |
| cihlová stěna | y 9–97, paralaxa poloviční rychlostí, cihly 12×6 |
| police 2. patra | horní hrana y 50, výška 6 (trubka s mechem) |
| police 1. patra | horní hrana y 74, výška 6 |
| chodník | y 98–103 (výška 6), kamenné dlaždice po 10 bodech, díry s vodou |
| voda | y 104–122, vlnky na hladině |
| krysa | levá hrana x 24, šířka 20, výška 12, nohy na y 98 |

Vzory generátoru: překážka na chodníku + odpadek nad ní; díra (16–26 bodů) + odpadek vysoko; police 1. patra (36–61) se 2–3 odpadky, někdy překážka pod ní a police 2. patra (28–45) s rolí toaletního papíru; pavouk na vlákně (délka 22–61, houpe se ±6) + odpadek na chodníku. Mezera mezi vzory 24–52 bodů, násobená poměrem rychlosti.

## Paleta (index, název, RGB)

Index 0 je průhledný. Ve spritech se používá znak → barva podle legendy níže.

| Znak | Barva | RGB |
|---|---|---|
| `k` | C_OUTLINE | 40, 24, 20 |
| `K` | C_BLACK | 10, 8, 12 |
| `g` | C_RAT | 142, 142, 152 |
| `G` | C_RAT_DARK | 92, 92, 102 |
| `l` | C_RAT_LIGHT | 202, 202, 212 |
| `p` | C_PINK | 240, 140, 150 |
| `w` | C_WHITE | 250, 250, 245 |
| `r` | C_RED | 220, 40, 40 |
| `y` | C_YELLOW | 250, 220, 60 |
| `o` | C_PIPE | 196, 110, 48 |
| `O` | C_PIPE_DARK | 130, 66, 28 |
| `L` | C_PIPE_LIGHT | 242, 172, 100 |
| `m` | C_MOSS | 70, 150, 60 |
| `M` | C_MOSS_DARK | 40, 100, 40 |
| `v` | C_SPIDER_LIGHT | 124, 84, 154 |
| `V` | C_SPIDER | 72, 40, 92 |
| `t` | C_THREAD | 205, 205, 205 |
| `s` | C_CAN | 200, 205, 215 |
| `S` | C_CAN_DARK | 120, 125, 140 |
| `R` | C_CAN_RED | 210, 50, 60 |
| `e` | C_PEAR | 182, 212, 70 |
| `E` | C_PEAR_DARK | 122, 152, 40 |
| `b` | C_BROWN | 112, 72, 30 |
| `c` | C_PAPER | 236, 230, 210 |
| `C` | C_PAPER_DARK | 182, 176, 160 |
| `n` | C_SLIME | 112, 222, 60 |
| `N` | C_SLIME_DARK | 62, 152, 40 |
| `x` | C_CRATE | 172, 122, 60 |
| `X` | C_CRATE_DARK | 112, 76, 36 |
| `H` | C_CRATE_LIGHT | 212, 162, 92 |
| `.` | průhledná | – |

Další barvy (pozadí, kreslené procedurálně): C_BRICK (122, 72, 58), C_BRICK2 (104, 60, 50), C_BRICK_DARK (78, 44, 40), C_MORTAR (58, 38, 36), C_CEIL (70, 64, 72), C_CEIL_DARK (44, 40, 48), C_CEIL_LIGHT (98, 92, 100), C_WATER (24, 70, 110), C_WATER_DARK (16, 48, 80), C_WATER_LIGHT (60, 120, 165), C_FOAM (175, 225, 240), C_STONE (128, 130, 120), C_STONE_DARK (86, 88, 82), C_STONE_LIGHT (172, 174, 162), C_MOSS_LIGHT (124, 204, 92), C_TEXT (250, 232, 120), C_TEXT_DARK (122, 102, 40), C_HUD_BG (30, 20, 30)

## Sprity (ASCII art, znak = 1 bod)

### SPR_RAT_RUN0 – krysa, běh fáze 1 – 20×12 bodů (60×36 px na displeji)

```
..............kkk...
.............kpppk..
......kkkkkkkkgppk..
p...kkggggggggggggk.
.p.kgggggggggggggggk
.pkgggggggggggggwKgk
kpgggggggggggggggggk
.kggggggggggggggggpk
..kkGGGGGGGGGGGGkkk.
....kGk.....kGk.....
....kk.......kk.....
....................
```

### SPR_RAT_RUN1 – krysa, běh fáze 2 – 20×12 bodů (60×36 px na displeji)

```
..............kkk...
.............kpppk..
......kkkkkkkkgppk..
.p..kkggggggggggggk.
p..kgggggggggggggggk
.pkgggggggggggggwKgk
kpgggggggggggggggggk
.kggggggggggggggggpk
..kkGGGGGGGGGGGGkkk.
......kGk.kGk.......
.......kk..kk.......
....................
```

### SPR_RAT_JUMP – krysa, skok – 20×12 bodů (60×36 px na displeji)

```
..............kkk...
.............kpppk..
......kkkkkkkkgppk..
....kkggggggggggggk.
...kgggggggggggggggk
..kgggggggggggggwKgk
.pkggggggggggggggggk
p.kggggggggggggggpk.
...kkGGGGGGGGGGGkk..
....kGGk...kGGk.....
.....kk.....kk......
....................
```

### SPR_SPIDER0 – pavouk, nohy fáze 1 – 11×9 bodů (33×27 px na displeji)

```
..k.....k..
...k...k...
..kkVVVkk..
.k.kVvVk.k.
k..kVVVk..k
...kvVvk...
..k.rkr.k..
.k..kkk..k.
k.........k
```

### SPR_SPIDER1 – pavouk, nohy fáze 2 – 11×9 bodů (33×27 px na displeji)

```
.k.......k.
..k.....k..
..kkVVVkk..
k..kVvVk..k
.k.kVVVk.k.
...kvVvk...
.k..rkr..k.
k...kkk...k
...........
```

### SPR_CAN – plechovka (odpadek) – 7×10 bodů (21×30 px na displeji)

```
.kkkkk.
kSsssSk
kssssSk
kSRRRSk
kSRRRSk
kSRRRSk
kssssSk
kSsssSk
kSsssSk
.kkkkk.
```

### SPR_PEAR – hruška (odpadek) – 8×10 bodů (24×30 px na displeji)

```
....kb..
...kb...
...kek..
..keeek.
..keeek.
.keeeeek
keeeeeEk
keeeeEEk
.keeEEk.
..kkkk..
```

### SPR_PAPER – zmuchlaný papír (odpadek) – 8×7 bodů (24×21 px na displeji)

```
..kkkk..
.kccccck
kcccCcck
kccCcCck
kcCccck.
.kcCcck.
..kkkk..
```

### SPR_ROLL – role toaletního papíru (odpadek) – 9×8 bodů (27×24 px na displeji)

```
..kkkkk..
.kwwwwwk.
kwwkkkwwk
kwkbbbkwk
kwkbbbkwk
kwwkkkwwk
.kwwwwwk.
..kkkkk..
```

### SPR_PIPE_STUB – rezavá trubka (překážka) – 10×14 bodů (30×42 px na displeji)

```
kkkkkkkkkk
kOoLooooOk
kOoLooooOk
kkkkkkkkkk
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
..kOLoOk..
```

### SPR_CRATE – bedna (překážka) – 12×11 bodů (36×33 px na displeji)

```
kkkkkkkkkkkk
kHxxxxxxxxHk
kxHxxxxxxHxk
kxxHxxxxHxxk
kxxxHHHHxxxk
kxxxHHHHxxxk
kxxHxxxxHxxk
kxHxxxxxxHxk
kHxxxxxxxxHk
kXXXXXXXXXXk
kkkkkkkkkkkk
```

### SPR_SLIME – louže slizu (překážka) – 16×5 bodů (48×15 px na displeji)

```
....kkkkkkk.....
..kknnnnnnnkk...
.knnnNnnnnNnnnk.
kNnnnnnnNnnnnnNk
kkkkkkkkkkkkkkkk
```

### SPR_HEART – srdíčko (HUD, plné) – 7×6 bodů (21×18 px na displeji)

```
.kk.kk.
krrkrrk
krrrrrk
.krrrk.
..krk..
...k...
```

### SPR_HEART_EMPTY – srdíčko (HUD, prázdné) – 7×6 bodů (21×18 px na displeji)

```
.kk.kk.
kKKkKKk
kKKKKKk
.kKKKk.
..kKk..
...k...
```

## Procedurálně kreslené prvky (bez spritu)

| Prvek | Rozměr | Popis |
|---|---|---|
| cihla | 12×6 | výplň C_BRICK / C_BRICK2 / C_BRICK_DARK podle hashe, spára C_MORTAR vpravo a dole |
| strop | 150×9 | C_CEIL, kameny 6×3 střídavě světlé/tmavé, krápník 1×3 pod každým čtvrtým |
| chodník | 150×6 | C_STONE, světlá horní a tmavá dolní linka, svislé spáry po 10 |
| díra | 16–26 × 6 | voda C_WATER se světlou hladinou, tmavé hrany |
| voda | 150×19 | C_WATER, vlnka C_WATER_LIGHT po sinusovce, odlesky |
| police (trubka) | 28–61 × 6 | C_PIPE, světlý pruh nahoře, tmavý dole, obrys, objímky po 14, mech 3×2 nahoře |
| vlákno pavouka | 1×N | C_THREAD od stropu k pavoukovi |
| rámeček úvodu / konce | 122×62 / 122×56 | výplň C_HUD_BG, dvojitý rám C_TEXT_DARK + C_TEXT, text 6×8 (velikost 1) a 12×16 (velikost 2) |

## Kdyby se grafika kreslila znovu (např. jiným nástrojem)

Zachovat rozměry v bodech (viz nadpisy spritů), průhlednost mimo tvar, 1bodový tmavý obrys, max. ~50 barev; krysa směřuje doprava, pavouk visí hlavou dolů s nohama do stran, překážky stojí na chodníku (spodní řádek spritu = y 97), odpadky se ve hře lehce pohupují (±1 bod).
