# esp32-rat-jumper — specifikace

Dětská skákačka pro Waveshare ESP32-S3-Touch-AMOLED-1.8, displej **na šířku** (tlačítko BOOT dole). Krysa běží kanálem zleva doprava, hráč skáče přes překážky, po policích se dostává do vyšších pater a sbírá odpadky k jídlu. Barevná pixel grafika ve stylu 16bitových skákaček (Prehistorik) přes barevný filtr starého CRT monitoru. Samostatná aplikace postavená na sdíleném kódu `../common/`, připravená na přidání do launcheru (modul `ratBegin/ratLoop/ratEnd`).

## Ovládání

- **Ťuknutí** kamkoli na displej = skok, **BOOT** = vysoký skok. Skok jde jen ze země nebo do `COYOTE_S` po sjetí z hrany.
- Na úvodní obrazovce a po konci hry ťuknutí (nebo BOOT) spustí novou hru.

## Svět

Logická plocha 150×123 bodů (1 bod = 3×3 px displeje). Odshora: strop s kameny a krápníky (`CEIL_H`), cihlová stěna s paralaxou (posouvá se poloviční rychlostí), dvě patra polic (`LANE2_Y`, `LANE1_Y`), chodník (`FLOOR_Y`) a pod ním voda s vlnkami. Krysa stojí vlevo (`RAT_X`), svět se posouvá rychlostí `SPEED_START` a zrychluje o `SPEED_GAIN` za sekundu až na `SPEED_MAX`.

Generátor před pravým okrajem náhodně přidává vzory a mezi ně mezeru `SPAWN_GAP_MIN..MAX` (roste s rychlostí):

- **překážka na chodníku** (rezavá trubka, bedna, louže slizu) s odpadkem nad ní,
- **díra v chodníku** (voda) s odpadkem vysoko nad ní,
- **police v 1. patře** (vodorovná trubka s mechem) s 2–3 odpadky, někdy překážka pod ní a police ve 2. patře s toaletním papírem,
- **pavouk** na vlákně ze stropu, houpe se; za ním odpadek na chodníku.

Odpadky: plechovka, hruška, zmuchlaný papír, role toaletního papíru; každý = 1 bod, lehce se pohupují.

## Pravidla

- 3 srdíčka. Náraz do překážky nebo pavouka, nebo pád do vody, ubere srdíčko a krysa `INVULN_S` bliká a je nezranitelná (po pádu do vody „plave" po hladině zpět na chodník).
- Bez srdíček konec: rámeček KONEC, body a nejlepší výsledek (od zapnutí), ťuknutí = znovu.

## Grafika

- Dvě sady spritů, volba před kompilací v `config.h` (`SPRITE_SET`): **KLASIK** (`rat_sprites_klasik.h`, ručně kreslené ASCII, prostředí procedurálně) a **ASTRA** (`rat_sprites_astra.h`, vygenerováno skriptem z balíčku `kanal-komplet.zip` od ChatGPT; obsahuje i dlaždice prostředí: cihly, strop, chodník, díry a voda ve 4 fázích, police s koncovkami a mechem, vlákno, panely, jiskra při sebrání, šplouchnutí). Z příkazové řádky: `--build-property compiler.cpp.extra_flags=-DSPRITE_SET=0` (0 = KLASIK, 1 = ASTRA).
- Sprity jsou ASCII art (znak → barva podle legendy, `.` průhledná), kreslí se přímo z řetězců. Rozměry v bodech: krysa 20×12 (dvě fáze běhu, skok), pavouk 11×9 (dvě fáze), plechovka 7×10, hruška 8×10, papír 8×7, role 9×8, trubka 10×14, bedna 12×11, sliz 16×5, srdíčko 7×6. Kompletní zadání pro grafika je v `prompt-grafika.md`.
- Paleta ~50 barev RGB888 (`PALETTE`), tmavé obrysy `k`, jasné výplně. Pozadí (cihly, strop, chodník, voda, police) se kreslí procedurálně primitivy Arduino_GFX do indexovaného canvasu (`Arduino_Canvas_Indexed`, režim přímých indexů).

## Barevný CRT filtr (bez PSRAM)

Canvas 150×123 × 8 bit (18 KB). Na displej po fyzických pruzích 368×32 přes vlastní SPI/DMA zařízení (jako hvězdy). Protože je obraz otočený, fyzický řádek displeje = logický sloupec hry: pro každý fyzický řádek se vezme jeden sloupec přes všechny logické řádky. Filtr:

- **stopa paprsku**: krajní třetiny bodu prolnuté se sousedním bodem ve směru logického x,
- **scanlines**: z trojice fyzických sloupců (logický řádek) je střední plný, horní `CRT_ROW_TOP`, dolní `CRT_ROW_BOT`, oba prosvícené sousedním řádkem (`CRT_ROW_BLEED`),
- **RGB maska**: trojice fyzických řádků v bodu nese po řadě červenou, zelenou a modrou; cizí kanály utlumené o `CRT_MASK`, jas kompenzuje `CRT_MASK_GAIN`,
- **vinětace** (`CRT_VIGNETTE`) a **blikání** (`CRT_FLICKER`).

`ROTATE_CW` v `config.h` otočí obraz o 180°, když je po nahrání vzhůru nohama.

## Zvuk

Přes sdílený `../common/amoled_audio.h` (ES8311 + I2S, task na jádře 0). Syntéza „8bitových" obdélníkových tónů a šumu: skok, vysoký skok, dopad, sebrání (dva tóny), zásah (šum + hluboký tón), pád do vody (bublavý šum), konec (tři klesající tóny), start. Hudba není.

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-rat-jumper.ino` | tenký wrapper: `hwInit()`, `touchBegin()`, `ratBegin()` / `touchRead()`, `ratLoop()` |
| `rat_app.h` | modul `ratBegin/ratLoop/ratEnd`, vstup (ťuknutí, BOOT) |
| `config.h` | rozměry světa, fyzika, pravidla, CRT, zvuk |
| `rat_palette.h` | paleta a legenda znaků spritů |
| `rat_sprites.h` | výběr sady; `rat_sprites_def.h` definice, `rat_sprites_klasik.h` / `rat_sprites_astra.h` sady |
| `rat_world.h` | stav hry, generátor vzorů, fyzika, kolize |
| `rat_render.h` | kreslení scény, HUD, úvod a konec |
| `rat_crt.h` | indexovaný canvas, barevný CRT filtr, přenos po pruzích (otočený obraz) |
| `rat_audio.h` | zvuky |

## Build

Stejné FQBN a port jako ostatní aplikace (viz `../../CLAUDE.md`), bez PSRAM.
