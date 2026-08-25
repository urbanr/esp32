# esp32-amoled-sand — specifikace

Interaktivní 2D falling-sand simulace pro Waveshare ESP32-S3-Touch-AMOLED-1.8. Fullscreen, řízená akcelerometrem (náklon = gravitace, třepání = rozptyl sypání), dotyk maže písek. Hardware a build viz kapitola zařízení v `../../CLAUDE.md`.

Referenční algoritmus je desktopová JS předloha (falling sand v HTML canvasu) — sekce „Referenční algoritmus" níže z ní přebírá přesnou mechaniku a výchozí hodnoty. Odlišnosti ESP32 verze: gravitace je plynulý vektor z IMU (ne fixně dolů), sypání je automatické podle náklonu (ne levým tlačítkem), dotyk pouze maže; funkce „rozpad kruhu z fotky" se nepřenáší.

## Hardware vstupy

| Zdroj | Čip | Přístup |
|---|---|---|
| Displej 368×448 | SH8601 (QSPI) | Arduino_GFX_Library, init dle CLAUDE.md |
| Dotyk | FT3168 | Arduino_DriveBus (`Arduino_FT3x68`), interrupt na `TP_INT` |
| Akcelerometr | QMI8658 | SensorLib (`SensorQMI8658.hpp`), stejná I2C (SDA 15, SCL 14) |
| Expander (napájení LCD) | XCA9554 @ 0x20 | Adafruit_XCA9554, piny 0–2 OUTPUT→HIGH s delay |

Piny z `pin_config.h` (knihovna Mylibrary). Logování HWCDC USBSerial.

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-amoled-sand.ino` | setup/loop, inicializace HW, hlavní tick smyčka |
| `config.h` | všechny laditelné parametry (viz níže) |
| `sand_sim.h` | stav simulace: grid, pravidla pádu, freeze/lepivost, emitter, mazání |
| `sand_render.h` | diferenciální render změněných buněk na displej |
| `sand_input.h` | čtení QMI8658 (vektor gravitace, detekce třepání) a FT3168 (touch) |
| `sand_palette.h` | generování pískové palety z kontrastu |

Kreslicí pomocné funkce berou cíl jako `Arduino_GFX *dst`.

## Grid a stav simulace

- Měřítko: `SAND_SCALE` display pixelů na zrnko (naladěno na **12×12** → grid 30×37; libovolná hodnota, při nedělitelném rozlišení se hrací plocha vycentruje a okraj dokryje pozadí).
- **Grid je jediný zdroj pravdy.** Každý viditelný pískový pixel odpovídá obsazené buňce gridu; render pouze zobrazuje diff gridu. Nikdy nesmí existovat pixel na displeji bez záznamu v gridu (zrnka by jím propadávala).
- Stav na buňku (po vzoru předlohy, oddělená pole):
  - `state` (uint8): `EMPTY` / `ACTIVE` / `FROZEN`
  - `calm` (uint8): počet klidných ticků, strop 250
  - `hold` (uint8): jak dlouho je zrnko „přilepené" lepivostí
  - `colorIdx` (uint8): index do pískové palety 0–10
- 4 × 41 KB ≈ 165 KB RAM — bez PSRAM se vejde (žádný framebuffer se nedrží). Kdyby bylo těsno, `calm`+`hold`+`colorIdx` lze sbalit do jednoho bajtu.
- Fixní krok simulace **60 Hz** (`TICK_DT = 1/60`) s akumulátorem času; render hned po ticku.

## Gravitace (plynulý vektor z IMU)

- Z akcelerometru se vezmou **složky v rovině displeje** (gx, gy) → směr pádu; funguje shodně displejem nahoru i dolů (kolmá složka se ignoruje).
- Velikost `m = |(gx,gy)| / 1g`: **0 = zařízení leží** (simulace stojí, nesype se, funguje jen mazání dotykem), **1 = displej svisle** (maximální rychlost). Pod mrtvou zónou `TILT_DEADZONE` se pohyb ani sypání nespouští.
- Pohyb zrnka za tick: pravděpodobnost pohybu úměrná `m`; „dolů" = losování mezi dvěma nejbližšími z 8 směrů podle úhlu vektoru (průměrný pohyb sleduje skutečný vektor, i šikmo). Diagonály skluzu jsou vztažené k aktuálnímu směru gravitace.
- Průchod gridu: od „nejnižší" strany podle aktuálního směru gravitace; kolmé pořadí se **střídá po ticích** (sudý tick zleva, lichý zprava — proti směrovému biasu, převzato z předlohy).
- Při otočení zařízení se všechen písek sype na druhou stranu — změna směru gravitace o víc než práh **probudí všechna zrnka**.

## Referenční algoritmus (převzato z JS předlohy)

Vyhodnocení jednoho `ACTIVE` zrnka v ticku, přesně v tomto pořadí:

1. **Pád rovně** ve směru gravitace: je-li cílová buňka `EMPTY`, přesun a konec (přesun vždy nuluje `calm` i `hold` cílové buňky a budí okolí uvolněné buňky).
2. **Dočasná lepivost:** pokud `hold < MAX_HOLD_TICKS` a náhoda < `STICKINESS_PROB`, zrnko tento tick drží: `hold++`, `calmStep()`, konec. Lepivost tedy zdržuje skluz, ale po vyčerpání `hold` už zrnko drženo být nemůže.
3. **Diagonální skluz:** pravděpodobnost `p = (calm == 0 ? SLIDE_SURFACE_PROB : SLIDE_SETTLED_PROB)` — čerstvě se pohybující zrnko klouže ochotně („po povrchu"), zrnko s `calm > 0` je součást usazené kupy a klouže málo. Při úspěchu se náhodně zvolí strana, zkusí se diagonála ve směru gravitace na této straně, pak na druhé; přesun do první volné.
4. **Zklidnění:** pokud se zrnko nepohnulo, `calmStep()`: `calm++` (strop 250) a při `calm >= CALM_TICKS_TO_FREEZE` přechod do `FROZEN` (zrnko se přestane vyhodnocovat — kupa se nerozplácává a šetří se výkon).

**Probouzení (`wakeAround`):** při každém uvolnění buňky (pohyb zrnka i smazání dotykem) se `FROZEN` sousedé v 5 buňkách na straně **proti gravitaci** a do stran (v předloze dy = −1..0, dx = −1..1) přepnou na `ACTIVE` s `calm = 0`, `hold = 0` — nic nezůstane viset ve vzduchu.

**Frakční akumulátory:** sypání i mazání počítají `acc += rate × TICK_DT`, provede se `floor(acc)` událostí, zbytek se přenáší; při puštění se akumulátor nuluje.

## Sypání (emitter)

- Sype se **jen při drženém BOOT tlačítku** (`POUR_BUTTON_PIN`, GPIO0) — a současně musí být zařízení nakloněné (vleže nesype ani s tlačítkem). Fyzika běží nezávisle na tlačítku.
- Sype se z **nejvyššího okraje** = okraj proti směru gravitace v rovině displeje.
- Střed sypací zóny = bod okraje nejblíž směru „vzhůru": horní okraj vodorovně → střed okraje; naklonění → posun k vyššímu rohu.
- Zóna je čtverec **10×10 display pixelů** (`EMIT_ZONE_BASE_PX`, jako v předloze) u okraje; každé zrnko se umístí na náhodnou pozici v zóně, jen do `EMPTY` buňky (obsazená pozice = zrnko se ten pokus nevysype).
- **Třepání** (high-pass odchylka |accel| od 1 g) zónu úměrně rozšiřuje (`SHAKE_ZONE_GAIN`).
- Rychlost sypání úměrná náklonu: `EMIT_MAX_PX_PER_S × m` zrnek/s při použití frakčního akumulátoru; vleže nula.
- Nové zrnko: `state = ACTIVE`, `calm = 0`, `hold = 0`, náhodný index palety (uniformně 0–10).
- Sypou se nová zrnka **a zároveň** gravitace působí na všechna už nasypaná.

## Mazání dotykem

- Držení prstu průběžně maže písek v zóně **10×10 display pixelů** kolem dotyku (`ERASE_ZONE_PX`, stejná mechanika jako sypání v předloze: každá mazací událost odstraní jednu náhodně vybranou buňku v zóně; prázdná pozice = událost propadne).
- Rychlost `TOUCH_ERASE_PX_PER_S` (rozsah 10–1000, výchozí 300) přes frakční akumulátor — v předloze jde o týž parametr „cursor sand px/s", který řídí sypání i mazání kurzorem.
- Každé smazání volá `wakeAround` — zrnka nad dírou se probudí a propadnou.
- Mazání funguje vždy, i když zařízení leží a simulace stojí.

## Písková paleta

- 11 barev: index 0 = základní barva, indexy 1–10 **tmavší** odstíny. Žádné světlejší.
- Vzorec z předlohy: kanály základní barvy × `f`, kde `f = 1 − contrast × (i/10) × MAX_DARKEN`, `MAX_DARKEN = 0.75`.
  - kontrast 0 % → všech 11 barev identických (jediná barva)
  - kontrast 100 % → nejtmavší odstín na 25 % jasu základní barvy
- Základní barva z předlohy: **RGB(230, 201, 122)** (~`0xE64F` v RGB565).
- Kontrast `PALETTE_CONTRAST_PCT` výchozí **80**. Paleta se předpočítá při startu do RGB565.
- Zrnko dostane náhodný index při vzniku a barvu si drží navždy.

## Render

- **Diferenciální, bez fullscreen canvasu** (368×448×2 B = 330 KB se bez PSRAM nevejde a není potřeba): simulace vede seznam změněných buněk; render je v jednom `startWrite()/endWrite()` bloku vykreslí — uvolněná buňka barvou pozadí, obsazená barvou palety, každá jako `SAND_SCALE`×`SAND_SCALE` blok.
- Žádné plošné mazání → žádné blikání (v souladu s flicker-free pravidly v CLAUDE.md; canvas tu nedává smysl, mění se rozptýlené jednotlivé buňky).
- Vzhled: fullscreen simulace, tmavé pozadí `BG_COLOR` (z předlohy vzduch RGB(22, 24, 29) ≈ `0x10C3`), čistý technický vzhled. Pevná země, stín u země a panel z desktopové předlohy odpadají — hranice tvoří okraje displeje, „zem" je vždy okraj po směru gravitace.

## Parametry v config.h

Tabulka níže uvádí hodnoty převzaté z předlohy; **aktuální naladěné hodnoty jsou v `config.h`** (po ladění na zařízení: `SAND_SCALE 12`, kontrast 60, `EMIT_MAX_PX_PER_S 50`, sypací zóna 20 px, mazací zóna 30 px, světlejší základní barva RGB(243, 222, 158), pozadí černé, sypání podmíněné BOOT tlačítkem).

| Parametr | Výchozí | Rozsah | Význam |
|---|---|---|---|
| `SAND_SCALE` | 2 | 1–2 | display pixelů na zrnko (2 = výkon, 1 = jemnost) |
| `TICK_HZ` | 60 | 20–120 | frekvence simulace (předloha: fixních 60) |
| `SLIDE_SURFACE_PROB` | 80 % | 0–100 | boční skluz po povrchu (padající zrnko, `calm == 0`) |
| `SLIDE_SETTLED_PROB` | 15 % | 0–100 | boční skluz ustálené kupy (`calm > 0`) |
| `CALM_TICKS_TO_FREEZE` | 30 | 1–120 | klidných ticků do zamrznutí zrnka |
| `STICKINESS_PROB` | 25 % | 0–100 | dočasná lepivost (šance držet, per tick) |
| `MAX_HOLD_TICKS` | 15 | 0–60 | maximální délka držení lepivostí (ticky) |
| `TOUCH_ERASE_PX_PER_S` | 300 | 10–1000 | rychlost mazání pod prstem („cursor sand px/s") |
| `ERASE_ZONE_PX` | 10 | — | strana mazací zóny 10×10 (display px) |
| `EMIT_MAX_PX_PER_S` | 400 | — | max. rychlost sypání (displej svisle) |
| `EMIT_ZONE_BASE_PX` | 10 | — | základní strana sypací zóny (display px) |
| `SHAKE_ZONE_GAIN` | 3.0 | — | zesílení rozšíření zóny třepáním |
| `TILT_DEADZONE` | 0.08 g | — | mrtvá zóna náklonu (pod ní simulace stojí) |
| `PALETTE_CONTRAST_PCT` | 80 | 0–100 | kontrast pískové palety |
| `MAX_DARKEN` | 0.75 | — | ztmavení nejtmavšího odstínu při 100% kontrastu (z předlohy) |
| `SAND_BASE_COLOR` | RGB(230, 201, 122) | — | základní barva písku (z předlohy) |
| `BG_COLOR` | RGB(22, 24, 29) | — | tmavé pozadí (z předlohy) |

## Build

Dle CLAUDE.md: `arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 esp32-amoled-sand`, upload na `/dev/cu.usbmodem1101`. Bez PSRAM (RAM rozpočet: 4 stavová pole ~165 KB + change list, žádný fullscreen framebuffer).
