# esp32-amoled-ship-navigator — specifikace

Dětská „palubní deska rakety" pro Waveshare ESP32-S3-Touch-AMOLED-1.8. Tři obrazovky ve stylu zeleného monitoru z 80. let (zelený fosfor na černé, hrubá mřížka 123×150 bodů s dosvitem, bloomem a scanlines), přepínané swipem doleva/doprava, se zvuky přes palubní reproduktor. Let je celý simulovaný, trvá 60 s a řídí se tlačítkem BOOT. Samostatná aplikace postavená na sdíleném kódu `../common/` (stejná inicializace hardwaru a dotyk jako ostatní aplikace), připravená na přidání do launcheru.

## Ovládání

- **BOOT** (`FLIGHT_BUTTON_PIN`, GPIO0): čekání → **start** letu; za letu → **pauza**; v pauze → **pokračování**; po dosažení cíle → vygeneruje se **nový let** (nové planety, nové rychlosti úbytku zásob) a hned startuje.
- **Swipe** doleva = další obrazovka, doprava = předchozí; z poslední se přechází na první a naopak. Swipe = dotyk s posunem alespoň `SWIPE_MIN_PX` převážně ve vodorovném směru, vyhodnocený při zvednutí prstu.

## Let (simulace)

- Trasa: start (raketa u dolního okraje) → planeta 1 → 2 → 3 → 4, celkem `FLIGHT_S = 60` s. Čas letu se rozděluje mezi úseky podle jejich délky na navigační obrazovce.
- Při každém novém letu se planety rozmístí **náhodně** (odspodu nahoru, v pásech, aby se nepřekrývaly), dostanou náhodná jména ze seznamu (MARS, LUNA, VENUS, TITAN, CERES, PLUTO, VESTA, EUROPA, IO, SATURN, MERKUR, NEPTUN, LOCHTA, STIPANA, PRASECI, PSI, OPACNA, MLHOVA, NOCNI, BUBLANINA, PIKOROVA, VLKOVA) a náhodné poloměry a krátery.
- **Zásoby** — jídlo, pití, palivo klesají ze 100 % na náhodnou koncovou hodnotu (`SUPPLY_END_MIN..MAX` %), toaleta se plní z 0 % na náhodnou hodnotu (`WASTE_END_MIN..MAX` %). Rychlosti se losují při startu každého letu.
- **Telemetrie** — hodnoty jsou odvozené z postupu letu s trochou šumu, aktualizují se `TELEMETRY_HZ`krát za sekundu, aby „tikaly" jako stará telemetrie: rychlost (profil zrychlení/brzdění v každém úseku), uletěná vzdálenost, kurz (směr aktuálního úseku), náklon X/Y, přetížení, úhel a vzdálenost Slunce, teplota pláště (podle úhlu ke Slunci), tlak kabiny, kyslík, radiace, palivo, tep posádky, cíl, zbývající vzdálenost, čas letu, stav.

## Obrazovky (kreslí se v nízkém rozlišení 123×150 bodů, 20 sloupců textu)

1. **LETOVE UDAJE** — hustá tabule 14 řádků `popisek … hodnota jednotka` (rychlost, uletěno, kurz, náklon, přetížení, Slunce, plášť, tlak, kyslík, radiace, palivo, tep, cíl, zbývá), hodnoty zarovnané vpravo. Bez závorek a jiných ozdob, které by ztěžovaly čtení.
2. **NAVIGACE** — planety jako kroužky s několika tečkami (krátery) a číslem/jménem, plánovaná trasa čárkovaně, uletěná část plnou čarou, raketa jako malý trojúhelník ve směru letu. Dole název dalšího cíle a vzdálenost.
3. **ZASOBY** — čtyři vodorovné pruhové grafy zleva doprava (JIDLO, PITI, PALIVO, TOALETA) ze segmentů, s procenty; pod 20 % (u toalety nad 80 %) bliká varování.

Společné: rámeček, titulní pruh, dole blikající stavová hláška s časem letu (STISKNI TLACITKO / LET 00:23 / PAUZA / CIL DOSAZEN) a indikátor obrazovek jako tři čtverečky (aktuální plný).

## Efekt staré obrazovky (bez PSRAM)

- Scéna se kreslí knihovnou Arduino_GFX do `Arduino_Canvas` 123×150 (37 KB) — jeden bod = 3×3 px displeje (`CRT_SCALE`). Barvy se používají jen jako intenzita zeleného fosforu (0–63).
- Na displej se přenáší po pruzích 368×32 vlastním SPI/DMA zařízením (stejný postup jako v `esp32-amoled-starfield/star_render.h`). Při skládání pruhu se na každý řádek bufferu aplikuje filtr podle toho, co dělají CRT shadery v emulátorech (scanlines, phosphor bloom/halation, persistence, vignette, flicker, noise):
  - **dosvit fosforu** (`CRT_DECAY`): jas bodu klesá postupně přes snímky, pohyb a změny textu nechávají krátkou stopu,
  - **bloom a záře** (`CRT_BLOOM`, `CRT_GLOW`): rozmazání přes 5 bodů a přídavek světla kolem jasných míst,
  - **stopa paprsku**: při zvětšení 3× se okraje bodu prolnou se sousedem, hrany jsou měkké,
  - **scanlines** (`CRT_ROW_TOP/BOT/BLEED`): z trojice řádků displeje je střední plný, horní a dolní tmavší a prosvícené sousedním řádkem bufferu,
  - **vinětace** (`CRT_VIGNETTE`): okraje a rohy tmavší (paprsek dopadal šikmo),
  - **blikání jasu** (`CRT_FLICKER`), **plovoucí světlejší pruh** (`CRT_HUM*`) a **náhodné jiskry** (`CRT_NOISE`),
  - paleta zeleného fosforu s gama (`CRT_GAMMA`) a lehkým nádechem do modra/červena.
- Celý displej se překresluje každý snímek (~30–40 fps), displej nikdy nevidí mezistav.

## Zvuk

Kodek ES8311 (I2C `0x18`) přes I2S (`ESP_I2S`, 16 kHz, 16 bit), zesilovač na pinu `PA`; inicializace kodeku převzatá z Waveshare příkladu `15_ES8311`. Zvuky se **syntetizují** v samostatném FreeRTOS tasku na jádře 0 (žádné soubory ve flash, vše laditelné v `config.h` a `ship_audio.h`):

- **motor** za letu: filtrovaný šum + hluboký tón, síla podle rychlosti (`AUDIO_ENGINE_GAIN`),
- **start**: narůstající hukot a stoupající tón (3 s),
- **minutí planety**: syčení manévrovacích trysek a dvě pípnutí (1,4 s),
- **přistání** na poslední planetě: klesající tón, syknutí a tupý dopad,
- **pauza / pokračování**: pípnutí.

Zvuk je volitelný: když kodek neodpoví, aplikace běží bez něj (log `ES8311 init fail`).

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-amoled-ship-navigator.ino` | tenký wrapper: `hwInit()`, `touchBegin()`, `shipBegin()` / `touchRead()`, `shipLoop()` |
| `ship_app.h` | modul `shipBegin()` / `shipLoop()` / `shipEnd()` (pro samostatný sketch i launcher) |
| `config.h` | parametry letu, zásob, swipe, CRT filtru, zvuku |
| `ship_sim.h` | stav letu: fáze, planety, trasa, zásoby, telemetrie |
| `ship_input.h` | swipe z dotyku, hrana tlačítka |
| `ship_screens.h` | kreslení tří obrazovek do bufferu nízkého rozlišení |
| `ship_crt.h` | buffer nízkého rozlišení, CRT filtr, přenos po pruzích přes DMA |
| `ship_audio.h` | ES8311 + I2S, syntéza zvuků v samostatném tasku |

## Build

Stejné FQBN a port jako ostatní aplikace (viz `../../CLAUDE.md`), bez PSRAM.
