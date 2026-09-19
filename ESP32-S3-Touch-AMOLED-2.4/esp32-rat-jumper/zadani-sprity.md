# Krysa skokan – zadání spritů pro grafika

Dětská skákačka (endless runner) na malém AMOLED displeji 368×448 px drženém na šířku. Krysa běží zleva doprava kanálem (stokou), skáče přes překážky, po policích z trubek vyskakuje do vyšších pater a sbírá odpadky k jídlu. Styl: barevný 16bitový pixel art jako Prehistorik – syté barvy, jednobodový tmavý obrys, jednoduché čitelné tvary, dětské a vtipné. Obraz jde přes filtr starého barevného CRT monitoru (scanlines, RGB maska), proto drobné detaily zanikají – kreslit hrubě a kontrastně.

## Technické podmínky

- Hra běží v **logické mřížce 150 × 123 bodů**, 1 bod = 3×3 px displeje. Všechny rozměry níže jsou v těchto bodech a jsou **závazné** (kód s nimi počítá pro kolize a umístění).
- Každý sprite jako samostatné **PNG v nativním rozlišení** (1 bod = 1 px), průhledné pozadí (alfa 0), bez antialiasingu, bez poloprůhlednosti. Zvětšovat jen nearest-neighbor pro náhled.
- Pojmenování souborů přesně podle tabulek (`png/<název>.png`, dlaždice `tiles/<název>.png`). Animace = číslované soubory `_0`, `_1`, …
- **Paleta max. 48 barev** pro celou sadu, ideálně tato (hex): outline #281814, black #100e16, gray #8e8e98, shade #5c5c66, light #cacad4, white #fafaf5, pink #f08c96, pinkshade #b85c76, purple #48285c, plight #7c549a, red #dc2828, metal #c8cdd7, cream #ece6d2, paper_shade #aaa89e, pear #b6d446, pear_shade #7a9828, brown #70481e, rust #c46e30, rust_shade #82421c, rust_light #f2ac64, wood #ac7a3c, wood_shade #704c24, wood_light #d4a25c, slime #70de3c, slime_shade #3e9828, brick1 #7a483a, brick2 #683c32, brick3 #4e2c28, mortar #3a2624, ceiling #464048, ceiling_light #605668, water #18466e, wave #3c78a5, deep #122e50, foam #a4dfec, stone #808278, stone_light #b0b2a0, stone_dark #505650, moss #46963c, yellow #fae878, gold #d4a23c, goldshade #82601c, silver #c8cdd7, silver_shade #8e8e98.
- Obrys všude stejnou barvou outline. Krysa a šnek směřují doprava. Věci stojící na chodníku mají spodní řádek spritu = úroveň chodníku (bez prázdných řádků dole). Věci visící ze stropu mají horní řádek = strop.
- Přiložit `palette.json` (název → hex) a `atlas.json` (název → w, h, poznámka o kotvení a animaci), jako v předchozích balíčcích.

## Scéna (kde se co používá)

| Prvek | Umístění (body) |
|---|---|
| strop | y 0–8 (výška 9), dlaždice `ceiling_repeat` s krápníky přesahujícími dolů |
| cihlová stěna | y 9–97, cihly 12×6, paralaxa poloviční rychlostí; na ní ozdoby (okno, ventil, mříž, lampa…) |
| police 2. patra | horní hrana y 50, výška 6 |
| police 1. patra | horní hrana y 74, výška 6 |
| chodník | y 98–103 (výška 6), dlaždice `walkway_repeat`; díry s vodou |
| voda | y 104–122, dlaždice `water_*` (4 fáze) |
| krysa | x 24, 20×12, nohy na y 98 |
| HUD | v pásu stropu: srdíčka vlevo, body vpravo, vedle bodů klíč, když ho krysa nese |

## Sprity postav a předmětů (`png/`)

| Soubor | Rozměr | Popis a chování ve hře |
|---|---|---|
| `rat_run_0`, `rat_run_1` | 20×12 | krysa, dvě fáze běhu (nohy), 10 fps |
| `rat_jump` | 20×12 | krysa ve skoku (natažená) |
| `spider_0`, `spider_1` | 11×9 | pavouk visící hlavou dolů, nohy do stran, dvě fáze, 4 fps |
| `can` | 7×10 | plechovka (odpadek, 1 bod) |
| `pear` | 8×10 | ohryzek hrušky (odpadek) |
| `paper` | 8×7 | zmuchlaný papír (odpadek) |
| `toilet_roll` | 9×8 | role toaletního papíru (odpadek) |
| `pipe` | 10×14 | rezavý pahýl trubky, překážka na chodníku |
| `crate` | 12×11 | dřevěná bedna, překážka |
| `slime` | 16×5 | louže slizu, překážka |
| `heart_full`, `heart_empty` | 7×6 | srdíčka v HUD |
| `cup_silver`, `cup_gold` | 10×12 | poháry; stříbrný každý 10. odpadek (3 body), zlatý každý 20. (10 bodů) |
| `gold_key` | 12×10 | zlatý klíč, sbírá se; v HUD vedle bodů; otevře příští bedýnku |
| `bubble_pickup_0`, `_1` | 10×10 | mýdlová bublina (sbíraná), průhledný vnitřek, dvě fáze odlesku, 4 fps |
| `bubble_shield_0`, `_1` | 26×20 | bublina kolem krysy, krysa uvnitř na offsetu (3,4); světlý obrys, průhledný vnitřek; kreslí se přes krysu |
| `rubber_duck_0`, `_1` | 24×14 | gumová kachnička plovoucí v díře, hlava vpravo; horní hrana těla x 3–13 na y 8 je pochozí (= chodník), čára ponoru y 12; dvě fáze pohupování |
| `snail_0`, `snail_1` | 14×9 | šnek lezoucí po chodníku proti kryse, dvě fáze tykadel, 6 fps |
| `drip_pipe` | 12×12 | trubka visící ze stropu, kapka se připojuje na x 6, y 12 |
| `drop_ready`, `drop_flash`, `drop_falling` | 5×7 | kapka: před pádem střídá ready/flash (varování), pak padá jako `falling` |
| `water_jet_pipe` | 14×8 | tryska stojící na chodníku, otvor nahoře uprostřed |
| `water_jet_repeat_0`…`_3` | 12×8 | segment proudu vody, opakuje se svisle nad tryskou, 4 fáze, 8 fps, neprůhledný uvnitř |
| `water_jet_top_0`…`_3` | 16×8 | rozstřik na vrcholu proudu (o 2 body širší na každou stranu než segment), 4 fáze |
| `reward_chest_closed`, `_open` | 14×13 | bedýnka s odměnou na chodníku; otevřená ukazuje poklad |

## Kulisy a ozdoby (`png/`) – **nové, zatím nakreslené jen provizorně**

Bez vlivu na hru, kreslit tlumeněji než sbírané předměty, aby nesplývaly s odpadky.

| Soubor | Rozměr | Popis |
|---|---|---|
| `bat_0`, `bat_1` | 15×6 | netopýr letící doleva, křídla nahoře / dole (mává, 10 fps), vlní se nahoru a dolů; tmavá silueta s červenýma očima |
| `wall_window` | 15×14 | kulaté zamřížované okno kanálu v cihlové zdi, kamenný prstenec, za mříží modravé světlo |
| `wall_valve` | 9×11 | ventil s kolem na krátké trubce vedoucí dolů |
| `wall_grate` | 12×8 | obdélníková mříž odpadu ve zdi, dole trochu mechu |
| `wall_lamp_0`, `wall_lamp_1` | 7×9 | blikající lampička na zdi (svítí / zhaslá) |
| `wall_cobweb` | 10×8 | pavučina do rohu (pod stropem) |
| `wall_roots` | 12×6 | kořínky prorůstající mezi cihlami |

## Dlaždice prostředí (`tiles/`) – už existují, uvádím pro úplnost

| Soubor | Rozměr | Popis |
|---|---|---|
| `brick_0`, `_1`, `_2` | 12×6 | tři varianty cihly se spárou vpravo a dole |
| `ceiling_repeat` | 24×12 | strop 9 bodů + krápníky přesahující 3 body dolů, opakovatelný vodorovně |
| `walkway_repeat` | 10×6 | dlaždice chodníku, opakovatelná |
| `hole_edge_left`, `_right` | 3×6 | hrany díry v chodníku |
| `water_0`…`_3` | 24×19 | voda pod chodníkem, 4 fáze vln, opakovatelná vodorovně |
| `hole_water_0`…`_3` | 24×6 | voda v díře chodníku, 4 fáze |
| `shelf_repeat` | 14×6 | police (trubka s objímkami), opakovatelná; `shelf_cap_left/right` 3×6 koncovky; `moss` 3×2 |
| `spider_thread` | 1×8 | vlákno pavouka, opakuje se svisle |
| `splash_0`…`_3` | 16×10 | šplouchnutí při pádu do vody |
| `pickup_spark_0`…`_3` | 7×7 | jiskra při sebrání |
| `panel_intro` | 122×62 | rámeček úvodní obrazovky (text KRYSA SKOKAN doplní hra) |
| `panel_game_over` | 122×56 | rámeček konce hry |

## Co chci dostat

ZIP s adresáři `png/` a `tiles/`, `palette.json`, `atlas.json`, `preview.png` (zvětšený přehled). Přednostně nové kulisy (netopýr, okno, ventil, mříž, lampa, pavučina, kořínky); ostatní jen pokud se překreslují celé v jednotném stylu.
