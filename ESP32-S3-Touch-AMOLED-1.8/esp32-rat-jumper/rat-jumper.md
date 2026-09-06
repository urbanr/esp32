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
- **pavouk** na vlákně ze stropu, houpe se; za ním odpadek na chodníku,
- **šnek** (`OB_SNAIL`) leze po chodníku proti kryse rychlostí `SNAIL_SPEED`, odpadek nad ním,
- **kapající trubka** u stropu: pauza `DRIP_WAIT_S`, kapka `DRIP_BLINK_S` bliká, pak padá rychlostí `DROP_SPEED` na chodník; zásah kapkou = srdíčko,
- **proud vody**: tryska na chodníku, proud až k `LANE2_Y`; krysa v něm stoupá rychlostí `JET_LIFT` na polici 2. patra s odpadky, která začíná hned za tryskou,
- **široká díra s kachničkou** (38–45): kachnička plave uprostřed, horní hrana těla je na úrovni chodníku (police 11 bodů široká), po ní se dá přeběhnout,
- za překážkou s pravděpodobností `CHEST_CHANCE` **bedýnka s odměnou**; otevře ji krysa, která nese klíč (`CHEST_PTS` bodů, klíč se spotřebuje).

Odpadky: plechovka, hruška, zmuchlaný papír, role toaletního papíru; každý = 1 bod, lehce se pohupují. Každý `CUP_EVERY`-tý odpadek (počítadlo `itemCount`) je **pohár**: stříbrný (`CUP_SILVER_PTS`) a každý druhý z nich zlatý (`CUP_GOLD_PTS`). Z ostatních je `KEY_CHANCE` % **zlatý klíč** (ukazuje se v HUD vedle bodů) a `BUBBLE_CHANCE` % **mýdlová bublina** (štít kolem krysy, viz pravidla).

Jen kulisa, bez vlivu na hru: na zdi se ve stejné paralaxe jako cihly opakují **ozdoby** (kulaté okno kanálu mezi stropem a 2. patrem, ventil mezi patry, mříž odpadu nad chodníkem; rozestup `WALL_DECO_STEP`, výběr a poloha z hashe pozice) a jednou za `BAT_GAP_MIN_S..MAX_S` prolétne **netopýr** zprava doleva (rychlost světa + `BAT_SPEED`), mává křídly a vlní se nahoru a dolů.

## Pravidla

- 3 srdíčka. Náraz do překážky, šneka, pavouka nebo kapky, nebo pád do vody, ubere srdíčko a krysa `INVULN_S` bliká a je nezranitelná (po pádu do vody „plave" po hladině zpět na chodník). Když má krysa bublinu, první zásah jen bublinu praskne (poloviční nezranitelnost, srdíčko zůstává).
- Bez srdíček konec: rámeček KONEC, body a nejlepší výsledek, ťuknutí = znovu.
- **Nejlepší výsledek na SD kartě** (`rat_hiscore.h`): SDMMC 1 bit (piny `SDMMC_CLK/CMD/DATA` z `pin_config.h`), soubor `/rat-jumper/best.txt` s jedním číslem; čte se při startu, zapisuje při novém rekordu na konci hry. Bez karty se skóre pamatuje jen od zapnutí.

## Grafika

- Dvě sady spritů, volba před kompilací v `config.h` (`SPRITE_SET`): **KLASIK** (`rat_sprites_klasik.h`, ručně kreslené ASCII včetně pohárů a doplňku, prostředí procedurálně) a **ASTRA** (`rat_sprites_astra.h`, vygenerováno skriptem `gen_astra.py` z balíčku `kanal-komplet.zip` od ChatGPT plus PNG v `astra-extra/png/` – poháry z obrázku uživatele a balíček `kanal-doplnek.zip`; obsahuje i dlaždice prostředí: cihly, strop, chodník, díry a voda ve 4 fázích, police s koncovkami a mechem, vlákno, panely, jiskra při sebrání, šplouchnutí). Generování: `python3 gen_astra.py <kanal-komplet> rat_sprites_astra.h rat_palette.h astra-extra`. Z příkazové řádky: `--build-property compiler.cpp.extra_flags=-DSPRITE_SET=0` (0 = KLASIK, 1 = ASTRA).
- Společné oběma sadám je `rat_sprites_extra.h` (netopýr, okno, ventil, mříž) s vlastní legendou `EXTRA_LEGEND` – používá jen znaky, které žádná sada nemá (generátor je má vyřazené z poolu), a barvy základní palety (stříbrná, zlatá, netopýr jsou v `rat_palette.h`). Poháry KLASIK jsou v `rat_sprites_cups.h` (ASTRA je má vygenerované, `SPRITES_HAVE_CUPS`).
- Sprity jsou ASCII art (znak → barva podle legendy, `.` průhledná), kreslí se přímo z řetězců. Rozměry v bodech: krysa 20×12 (dvě fáze běhu, skok), pavouk 11×9 (dvě fáze), plechovka 7×10, hruška 8×10, papír 8×7, role 9×8, trubka 10×14, bedna 12×11, sliz 16×5, srdíčko 7×6; poháry 10×12 (ASTRA) / 11×10 a 13×12 (KLASIK); klíč 12×10, bublina 10×10 (2 fáze), štít 26×20 (krysa na offsetu 3,4), kachnička 24×14 (2 fáze), šnek 14×9 (2 fáze), kapající trubka 12×12, kapka 5×7 (připravená / blikající / padající), tryska 14×8, segment proudu 12×8 a čepice 16×8 (4 fáze), bedýnka 14×13 (zavřená / otevřená); netopýr 15×6 (2 fáze), okno 15×14, ventil 9×11, mříž 12×8. Kompletní zadání pro grafika je v `prompt-grafika.md`.
- Paleta ~50 barev RGB888 (`PALETTE`), tmavé obrysy `k`, jasné výplně. Pozadí (cihly, strop, chodník, voda, police) se kreslí procedurálně primitivy Arduino_GFX do indexovaného canvasu (`Arduino_Canvas_Indexed`, režim přímých indexů).

## Barevný CRT filtr (bez PSRAM)

Canvas 150×123 × 8 bit (18 KB). Na displej po fyzických pruzích 368×32 přes vlastní SPI/DMA zařízení (jako hvězdy). Protože je obraz otočený, fyzický řádek displeje = logický sloupec hry: pro každý fyzický řádek se vezme jeden sloupec přes všechny logické řádky. Filtr:

- **scanlines**: z trojice fyzických sloupců (logický řádek) je střední plný, horní `CRT_ROW_TOP`, dolní `CRT_ROW_BOT`,
- **RGB maska**: trojice fyzických řádků v bodu nese po řadě červenou, zelenou a modrou; cizí kanály utlumené o `CRT_MASK`, jas kompenzuje `CRT_MASK_GAIN`,
- **vinětace** (`CRT_VIGNETTE`, výchozí 0 = vypnuto) a **blikání** (`CRT_FLICKER`).

Dva režimy (`CRT_SOFT` v `config.h`): **rychlý** (výchozí, ~34 fps) má pixel = jedno čtení předpočítané tabulky paleta × scanline × maska × vinětace × blikání; **měkký** (`CRT_SOFT 1`, ~16 fps) navíc prosvěcuje sousední řádky (`CRT_ROW_BLEED`) a prolíná krajní třetiny bodu se sousedním sloupcem (stopa paprsku). Sketch se překládá s `-O2` (`build_opt.h`); s výchozím `-Os` byl rychlý režim o třetinu pomalejší.

`ROTATE_CW` v `config.h` otočí obraz o 180°, když je po nahrání vzhůru nohama.

## Zvuk

Přes sdílený `../common/amoled_audio.h` (ES8311 + I2S, task na jádře 0). Syntéza „8bitových" obdélníkových tónů a šumu: skok, vysoký skok, dopad, sebrání (dva tóny), pohár a bedýnka (tři stoupající tóny), prasknutí bubliny, šum proudu vody, zásah (šum + hluboký tón), pád do vody (bublavý šum), konec (tři klesající tóny), start. Hudba není. Hlasitost `AUDIO_VOLUME` (kodek) a `AUDIO_MASTER`.

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-rat-jumper.ino` | tenký wrapper: `hwInit()`, `touchBegin()`, `ratBegin()` / `touchRead()`, `ratLoop()` |
| `rat_app.h` | modul `ratBegin/ratLoop/ratEnd`, vstup (ťuknutí, BOOT) |
| `config.h` | rozměry světa, fyzika, pravidla, CRT, zvuk |
| `rat_palette.h` | paleta a legenda znaků spritů |
| `rat_sprites.h` | výběr sady; `rat_sprites_def.h` definice, `rat_sprites_klasik.h` / `rat_sprites_astra.h` sady, `rat_sprites_extra.h` společné (netopýr, ozdoby), `rat_sprites_cups.h` poháry KLASIK |
| `gen_astra.py`, `astra-extra/` | generátor sady ASTRA z PNG; PNG mimo základní balíček (poháry, doplněk) |
| `rat_hiscore.h` | nejlepší skóre na SD kartě (SDMMC 1 bit) |
| `rat_world.h` | stav hry, generátor vzorů, fyzika, kolize |
| `rat_render.h` | kreslení scény, HUD, úvod a konec |
| `rat_crt.h` | indexovaný canvas, barevný CRT filtr, přenos po pruzích (otočený obraz) |
| `rat_audio.h` | zvuky |

## Build

Stejné FQBN a port jako ostatní aplikace (viz `../../CLAUDE.md`), bez PSRAM.
