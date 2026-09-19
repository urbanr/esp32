# esp32-amoled-bubble-level — specifikace

Kruhová vodováha pro Waveshare ESP32-S3-Touch-AMOLED-1.8. Stejný hardwarový základ a knihovny jako `../esp32-amoled-sand/` (viz `sand.md` a kapitola zařízení v `../../CLAUDE.md`): displej SH8601 přes Arduino_GFX_Library, akcelerometr QMI8658 přes SensorLib, expander XCA9554, logování HWCDC USBSerial, I2C recovery před `Wire.begin()` (převzít `i2cBusRecover()` ze sand projektu). Dotyk se nepoužívá.

## Vzhled

Barevně a materiálově podle skutečné kruhové vodováhy (zelená kapalina, sklo, černá ryska) — ne doslovná kopie předlohy, ale její barvy a přechody, aby výsledek působil trochu realisticky. Vrstvy odspodu nahoru:

1. **Pouzdro** — vše mimo kruh vodováhy černé (`CASE_COLOR`); na AMOLED splyne s rámečkem a kulaté „sklo" vynikne.
2. **Zelená kapalina** — kruh přes celou šířku displeje (`LIQUID_RADIUS_PX = 184`, střed displeje) s **radiálním přechodem**: nejsvětlejší žlutozelená uprostřed (`LIQUID_CENTER_COLOR`), plynule do tmavší zelené u okraje (`LIQUID_EDGE_COLOR`). U samého okraje tenký tmavý přechodový prstenec (`GLASS_RIM_DARK`) a na něm světlý oblouk odlesku vlevo nahoře (`GLASS_RIM_LIGHT`) — dojem zaobleného skla.
3. **Bublina** — pohyblivá čočka jako na předloze: výplň jen o málo světlejší než okolní kapalina (`BUBBLE_FILL_COLOR`), tmavší zelený okraj (`BUBBLE_EDGE_COLOR`, tloušťka `BUBBLE_EDGE_THICKNESS_PX`), uvnitř okraje tenký světlý lem a světlý srpek odlesku v horní části (`BUBBLE_HIGHLIGHT_COLOR`).
4. **Černá kružnice** — statická referenční kružnice ve středu (`TARGET_COLOR`, černá; poloměr `TARGET_RADIUS_PX`, tloušťka `TARGET_THICKNESS_PX`). Je **vždy navrchu** — bublina podjíždí pod ní. Když je zařízení vyrovnané, bublina sedí uvnitř kružnice.
5. **Údaj náklonu** — malý text v levém horním rohu (v černé ploše pouzdra mimo kruh kapaliny): `X: +12.3°` / `Y: -0.4°` na dvou řádcích.

Rozměry (výchozí, vše v config.h): bublina poloměr `BUBBLE_RADIUS_PX = 46` px, černá kružnice poloměr `TARGET_RADIUS_PX = 62` px (bublina se do ní s rezervou vejde). Střed dráhy = střed displeje (184, 224). Maximální výchylka středu bubliny `R_MAX_PX` = tak, aby bublina zůstala celá uvnitř kapaliny: `LIQUID_RADIUS_PX − BUBBLE_RADIUS_PX − šířka okrajových prstenců` ≈ 128 px.

## Čtení náklonu (QMI8658)

- Z akcelerometru vektor (ax, ay, az); složky v rovině displeje (gx, gy) určují **směr** výchylky, úhel náklonu `θ = asin(clamp(|(gx,gy)| / |g|, 0, 1))`.
- Bublina se vychyluje **proti** směru gravitace v rovině displeje — ke kraji, který jde nahoru (bublina plave).
- Údaje pro textový výpis: `náklon X = asin(gx/|g|)`, `náklon Y = asin(gy/|g|)` ve stupních, znaménko tak, aby kladné X odpovídalo zvednutému pravému okraji a kladné Y zvednutému hornímu okraji.
- Surová data z IMU lehce filtrovat (klouzavý průměr / low-pass), aby textový údaj nekmital.
- **Aretace:** stisk tlačítka BOOT (`ZERO_BUTTON_PIN`, GPIO0) vezme aktuální filtrovaný náklon jako novou rovinu — náklon každé osy se dál měří jako rozdíl úhlů (`asin(gx) − asin(gx₀)`), takže je přesný pro náklon kolem jedné osy a symetrický i při větším náklonu referenční roviny; bublina jde do středu a údaj náklonu ukáže 0. Filtr IMU se po startu nastaví prvním vzorkem a stisk držený už při startu se nepočítá. Posun se nepamatuje přes restart ani přes přepnutí aplikace v launcheru (čistý start).

## Mapování náklonu na výchylku (nelineární „vyboulené sklo")

- Normalizovaný náklon `u = clamp(θ / TILT_FULL_SCALE_DEG, 0, 1)`; plná výchylka při `TILT_FULL_SCALE_DEG = 20°`.
- Výchylka středu bubliny: `r = R_MAX_PX × pow(u, BUBBLE_CURVE_EXP)` ve směru „nahoru".
- `BUBBLE_CURVE_EXP < 1` dává požadované chování jako u vypouklého skla: kolem nuly citlivé (malý náklon = znatelný pohyb), ke kraji stále méně citlivé. Výchozí **0.5**; 1.0 = lineární.

## Pohyb bubliny — dva režimy (přepínač v config.h)

`BUBBLE_MOTION_MODE`:

- **`MOTION_SMOOTH`** (výchozí) — pozice je přímo cíl z mapování, dohlazený exponenciálním filtrem: `pos += (target − pos) × SMOOTH_ALPHA` za tick (`SMOOTH_ALPHA` výchozí 0.15). Klidné, bez překmitu.
- **`MOTION_PHYSICS`** — bublina má rychlost; pružina k cíli + tlumení: `vel += (target − pos) × SPRING_K × dt; vel × = DAMPING; pos += vel × dt`. Při rychlém naklonění mírně přestřelí a zhoupne se. Parametry `SPRING_K` (výchozí 40 /s²) a `DAMPING` (výchozí 0.90 za tick) naladit na zařízení.

Fixní krok smyčky `TICK_HZ = 60` s akumulátorem času (stejně jako sand).

## Render (flicker-free, bez PSRAM)

Dle pravidel v CLAUDE.md — žádné „smaž → nakresli" přímo na displej:

- **Pozadí není jednolitá barva, ale funkce:** `liquidColorAt(x, y)` vrací barvu radiálního přechodu kapaliny (mimo kruh kapaliny `CASE_COLOR`). Implementace přes předpočítanou LUT 256 odstínů `LIQUID_CENTER_COLOR → LIQUID_EDGE_COLOR` indexovanou normalizovaným poloměrem — stejná funkce se použije při úvodním fullscreen vykreslení i při každém překreslení dirty oblasti pod bublinou.
- **Statika jednou v `setup()`:** pouzdro, celá kapalina s přechodem, odlesky skla u okraje, černá kružnice, popisky. Přímo přes `gfx`.
- **Bublina přes malý pohyblivý canvas:** fullscreen canvas (330 KB) se bez PSRAM nevejde a není potřeba. Jeden `Arduino_Canvas` o straně `BUBBLE_CANVAS_PX = 128` px (≈ 32 KB RGB565). Každý snímek:
  1. dirty obdélník = sjednocení bboxu bubliny na staré a nové pozici (vyhlazený pohyb je pomalý, vejde se do canvasu; kdyby ne — ve fyzikálním režimu — rozdělit na dva kroky: smaž starou, nakresli novou),
  2. do canvasu vykreslit v pořadí: výplň přes `liquidColorAt` po pixelech → bublina (výplň, okraj, lem, odlesk) → **část černé kružnice zasahující do obdélníku** (aby zůstala navrchu),
  3. poslat na displej jedním `gfx->draw16bitRGBBitmap(x, y, buffer, w, h)` výřezem dirty obdélníku (canvas s pevným offsetem nestačí, pozice se mění).
- Bublina se pohybuje jen uvnitř kapaliny (`R_MAX_PX`), takže dirty oblast nikdy nezasahuje do statických odlesků u okraje skla.
- **Text náklonu přes vlastní malý canvas** v rohu (např. 120×48 px), s pevným offsetem a `flush()`. Překreslovat jen při změně zobrazené hodnoty (krok 0.1°), max ~5 Hz.
- `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)` až po `gfx->begin()`, kontrola návratové hodnoty (viz KRITICKÉ v CLAUDE.md).
- Kreslicí pomocné funkce berou cíl jako `Arduino_GFX *dst`.

## Struktura souborů

Aplikace je rozdělená na modul a sketch: modul `level_app.h` poskytuje `levelBegin()` / `levelLoop()` / `levelEnd()` a používá ho jak samostatný sketch `esp32-amoled-bubble-level.ino`, tak launcher (`../esp32-amoled-launcher/`, kde se aplikace přepínají dvojklikem). Inicializace hardwaru (USBSerial, I2C recovery, expander, SPI sběrnice, displej, jas `AMOLED_BRIGHTNESS`) je sdílená v `../common/amoled_hw.h` a volá ji sketch; modul kreslí jen přes `gfx`. `levelBegin()` = plná inicializace IMU a překreslení scény, canvasy se alokují v `levelBegin()` a uvolňují v `levelEnd()`, aby po přepnutí neblokovaly RAM jiné aplikaci.

| Soubor | Obsah |
|---|---|
| `esp32-amoled-bubble-level.ino` | tenký wrapper: `hwInit()` z `../common/amoled_hw.h`, `levelBegin()` / `levelLoop()` |
| `level_app.h` | modul aplikace: `levelBegin()` / `levelLoop()` / `levelEnd()`, pohyb bubliny, hlavní smyčka |
| `config.h` | všechny laditelné parametry (viz níže) |
| `level_input.h` | čtení QMI8658, filtrace, výpočet θ, směru a náklonů X/Y |
| `level_render.h` | kreslení bubliny, kružnice, textu; správa canvasů a dirty obdélníku |

## Parametry v config.h

| Parametr | Výchozí | Význam |
|---|---|---|
| `TILT_FULL_SCALE_DEG` | 20 | náklon, při kterém je bublina na kraji (plná výchylka) |
| `BUBBLE_CURVE_EXP` | 0.5 | exponent nelineární křivky (<1 = citlivé u středu, 1 = lineární) |
| `BUBBLE_MOTION_MODE` | `MOTION_SMOOTH` | `MOTION_SMOOTH` / `MOTION_PHYSICS` |
| `SMOOTH_ALPHA` | 0.15 | síla dohlazení v režimu SMOOTH (za tick) |
| `SPRING_K` | 40 | tuhost pružiny v režimu PHYSICS (/s²) |
| `DAMPING` | 0.90 | tlumení rychlosti v režimu PHYSICS (za tick) |
| `TICK_HZ` | 60 | frekvence smyčky |
| `BUBBLE_RADIUS_PX` | 46 | poloměr bubliny |
| `BUBBLE_EDGE_THICKNESS_PX` | 3 | tloušťka tmavšího okraje bubliny |
| `TARGET_RADIUS_PX` | 62 | poloměr referenční kružnice |
| `TARGET_THICKNESS_PX` | 3 | tloušťka referenční kružnice |
| `LIQUID_RADIUS_PX` | 184 | poloměr kruhu kapaliny (celá šířka displeje) |
| `BUBBLE_CANVAS_PX` | 128 | strana canvasu bubliny (musí pojmout bublinu + pohyb za snímek) |
| `CASE_COLOR` | RGB(0, 0, 0) | pouzdro — plocha mimo kruh kapaliny |
| `LIQUID_CENTER_COLOR` | RGB(178, 214, 90) | kapalina uprostřed (světlá žlutozelená) |
| `LIQUID_EDGE_COLOR` | RGB(105, 150, 45) | kapalina u okraje (tmavší zelená) |
| `GLASS_RIM_DARK` | RGB(60, 90, 30) | tmavý prstenec u okraje skla |
| `GLASS_RIM_LIGHT` | RGB(220, 235, 190) | světlý oblouk odlesku skla (vlevo nahoře) |
| `BUBBLE_FILL_COLOR` | RGB(198, 226, 120) | výplň bubliny (o málo světlejší než kapalina) |
| `BUBBLE_EDGE_COLOR` | RGB(95, 140, 45) | tmavší zelený okraj bubliny |
| `BUBBLE_HIGHLIGHT_COLOR` | RGB(240, 248, 215) | světlý lem a srpek odlesku na bublině |
| `TARGET_COLOR` | RGB(0, 0, 0) | černá referenční kružnice |
| `TILT_TEXT_COLOR` | RGB(140, 160, 120) | barva textu náklonu (na černém pouzdru) |

## Build

Dle CLAUDE.md, bez PSRAM:

```sh
arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 esp32-amoled-bubble-level
arduino-cli upload  -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 -p /dev/cu.usbmodem1101 esp32-amoled-bubble-level
```
