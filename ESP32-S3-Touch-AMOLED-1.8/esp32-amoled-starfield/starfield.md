# esp32-amoled-starfield — specifikace

Průlet hvězdným polem pro Waveshare ESP32-S3-Touch-AMOLED-1.8. Stejný hardwarový základ jako `../esp32-amoled-sand/` a `../esp32-amoled-bubble-level/` (viz kapitola zařízení v `../../CLAUDE.md`): displej SH8601 přes Arduino_GFX_Library, IMU QMI8658 přes SensorLib, expander XCA9554, logování HWCDC USBSerial, I2C recovery před `Wire.begin()`. Dotyk se nepoužívá. Bez PSRAM.

## Chování

- Hvězdy jsou body ve **světě** (ne na displeji). Letí vodorovně pevným světovým směrem; ve výchozím stavu **přímo na diváka**. Co hvězda, to bod (velikost viz níže).
- Zařízení je **kamera**: oko v počátku, dívá se kolmo do displeje, perspektiva podle `EYE_DIST_PX`. Natočení zařízení se čte z IMU, takže hvězdy se vůči reálnému světu pohybují pořád stejně a zařízením se na ně díváš z různých úhlů:
  - svisle před očima → klasický star field (hvězdy vylétají ze středu),
  - na plocho displejem nahoru → hvězdy tečou přes displej od jedné hrany ke druhé (od hrany, která je „dál od tebe"),
  - mezi tím → šikmý průlet.
- Každá hvězda je čtverec `STAR_SIZE_PX` × `STAR_SIZE_PX` px (výchozí 2, protože 1 px je na tomto displeji téměř neviditelný). Hvězdy s jasem pod `STAR_MIN_BRIGHT` se nekreslí.
- **Hloubka:** hvězda se rodí na okraji koule `FIELD_RADIUS` kolem oka jako černá, s přibližováním jasní až k bílé (resp. svému odstínu), proletí kolem diváka, za ním zase tmavne a při opuštění koule se objeví na druhé straně na nové náhodné pozici. Jas `= (1 − d/R)^BRIGHT_GAMMA`, kde `d` je vzdálenost od oka; rozložení hvězd v kouli je rovnoměrné → většina hvězd je daleko a tmavá, pole působí hluboce.
- **Barvy:** část hvězd (`STAR_TINT_PROB_PCT`) dostane náhodný nádech k jedné z barev palety `STAR_TINT_COLORS` (bílá → žlutá → oranžová), síla nádechu náhodně 0..`STAR_TINT_MAX`.

## Orientace (QMI8658, akcelerometr + gyroskop)

Souřadnice displeje: +X doprava, +Y dolů, +Z do displeje (od diváka). Sledují se dva jednotkové vektory:

- `gDir` — směr gravitace. Komplementární filtr: predikce gyrem (`g -= (ω×g)·dt`), korekce akcelerometrem s vahou `GRAVITY_FILTER_ALPHA` za snímek.
- `flowDir` — vodorovný směr letu hvězd. Otáčí se gyrem, po každém kroku se promítne kolmo na `gDir` (drží se vodorovný). Bez gyra by nešlo poznat otočení kolem svislé osy (na plocho otočíš zařízení na stole a akcelerometr vidí pořád totéž).
- **Návrat k výchozímu směru:** `flowDir` se rychlostí `RECENTER_RATE` (1/s) přitahuje k vodorovnému průmětu normály displeje (= „přímo na mě"), síla úměrná čtverci velikosti tohoto průmětu (svisle plná, se sklápěním slábne). Leží-li zařízení (průmět < `FLAT_LIMIT`), návrat není a směr drží jen gyro. Maže to drift gyra; `RECENTER_RATE 0` = čistě gyro.
- **Start:** `flowDir` = vodorovný průmět normály; leží-li zařízení, směr „od horní hrany k dolní" (+Y).
- **Bias gyra** se měří jako průměr `GYRO_CALIB_SAMPLES` po sobě jdoucích vzorků, kdy je zařízení v klidu (|ω| < `GYRO_STILL_DPS`, změna akcelerace mezi vzorky < `ACCEL_STILL_G`); jakýkoli pohyb okno restartuje. Měří se **kdykoli** zařízení takto dlouho leží (ne jen po startu), do prvního změření se gyro nepoužívá (jen akcelerometr — hvězdy letí na diváka). Absolutní |a| se nepoužívá: tento kus ukazuje v klidu ~0,91–0,99 g, surový bias gyra ~4,5 dps.
- Báze hvězdného pole v souřadnicích displeje: `W = −flowDir` (odkud hvězdy letí), `V = −gDir` (nahoru), `U = V × W`. Hvězda `(u, v, w)` → `p = uU + vV + wW`; hloubka `p.z`, průmět `x = CX + p.x·EYE_DIST/p.z`.

### Mapování os senzoru (config.h)

X/Y akcelerometru převzato ze sand/bubble-level (ověřeno: směr gravitace v rovině displeje). Osa Z a gyro jsou dopočítány jako **tatáž rotace** (senzor hlásí reakci, gravitace = −a; z toho plyne rotace senzor→displej o −90° kolem Z, tedy `gyro_d = (gy, −gx, gz)`, `g_z = −az`). Ověřeno na zařízení: na plocho displejem nahoru `az ≈ −0,97` → `ACCEL_MAP_GZ ≈ +1` (do displeje). Kontrola gyra za běhu v logu: `gsign` musí být při pohybu **kladné**, `gerr` v jednotkách stupňů. Kdyby ne, alternativa je v komentáři v `config.h`.

## Render (flicker-free, bez PSRAM, DMA)

Celý displej je dynamický, fullscreen canvas (330 KB) se bez PSRAM nevejde. Řešení: **plné překreslení každý snímek po vodorovných pruzích** `LCD_WIDTH × STRIPE_H` (32 px → 14 pruhů, 2 ping-pong buffery po 23,5 KB): pruh se vynuluje, vykreslí se do něj body z projekce (RGB565 už s prohozenými bajty) a pošle se jako **jedna DMA transakce** přes vlastní SPI zařízení (`spi_device_queue_trans`); zatímco se přenáší, CPU skládá další pruh. Každý pruh je samostatný zápis stejnou sekvencí, jakou posílá Arduino_GFX: CASET, PASET (řádky pruhu), RAMWR (opcode `0x02`) a data (opcode `0x32`, adresa `0x003C00`, pixely po 4 linkách). Displej nikdy nevidí mezistav.

Proč ne `draw16bitRGBBitmap` z Arduino_GFX: knihovna posílá po 1024px transakcích s pollingem (CPU čeká) a přehazuje bajty za běhu — změřeno **~40 ms na snímek** (22 fps) při 40 MHz, 32 ms při 80 MHz. S DMA: **render 14,7 ms, snímek ~19 ms → ~52 fps** při 40 MHz (přenos ~16,5 ms je teoretické minimum QSPI 40 MHz; poslední pruh dobíhá během simulace dalšího snímku). `LCD_QSPI_HZ 80000000` by dal odhadem ~80 fps — nutno ověřit obraz okem.

Vazby na knihovnu: SPI bus inicializuje projekt sám (`renderBusInit()`, větší `max_transfer_sz`), `Arduino_ESP32QSPI` se vytváří s `is_shared_interface = true` (jinak si bus natrvalo zamkne) a `gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN)` jen inicializuje panel a nastaví jas. `renderInit()` pak přidá vlastní zařízení s HW CS na `LCD_CS` — **od té chvíle se přes `gfx` nesmí kreslit** (knihovna řídí CS ručně přes GPIO, které už patří SPI).

Změřeno (`STAR_COUNT 1800`, ~300 viditelných bodů): projekce ~1,7 ms, simulace + čtení IMU po I2C ~2,6 ms, render 14,7 ms. Počet hvězd fps prakticky neovlivní.

## Diagnostika (USBSerial, každých `DEBUG_PERIOD_MS`)

`USBSerial.setTxTimeoutMs(0)` je nutné: bez toho výpis blokuje smyčku, když port nikdo nečte (viz Známé pasti v `../../CLAUDE.md`) — projevovalo se jako ~1 fps, které „zmizelo" při otevření sériového monitoru.


```
fps 22.6  us sim 2870 proj 885 render 40491  imu 1 calib 1  |a| 0.968 |w| 4.55 az -0.97  gerr 0.1 gsign +0.000  g(-0.04 +0.00 +1.00)  flow(+0.03 +1.00 -0.01)  dots 302
```

`sim/proj/render` = µs na snímek (render = skládání pruhů + čekání na DMA); `calib` = bias gyra změřen; `|a|`, `|w|` = surové velikosti (ladění prahů klidu); `az` = surová osa Z (kontrola `ACCEL_MAP_GZ`); `gerr` = max. rozdíl gravitace gyro vs. akcelerometr ve stupních za periodu (v klidu ~0, při pohybu jednotky stupňů); `gsign` = korelace změny gravitace s predikcí gyra (při pohybu kladné = znaménka gyra OK); `dots` = počet vykreslených hvězd.

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-amoled-starfield.ino` | setup/loop, inicializace HW (expander, displej, IMU, I2C recovery), diagnostika |
| `config.h` | všechny laditelné parametry |
| `star_input.h` | čtení QMI8658, kalibrace biasu, fúze gravitace + směru letu, báze pole |
| `star_field.h` | hvězdy v kouli: spawn, pohyb, barva |
| `star_render.h` | SPI bus + DMA zařízení, projekce kamerou, jas, pruhový render |

## Parametry v config.h

| Parametr | Výchozí | Význam |
|---|---|---|
| `STAR_COUNT` | 2700 | celkový počet simulovaných hvězd (v zorném poli je jich cca 1/6) |
| `FIELD_RADIUS` | 1500 | poloměr koule hvězd kolem oka (jednotky = display px) |
| `STAR_SPEED` | 900 | rychlost letu (jednotky/s) |
| `EYE_DIST_PX` | 200 | vzdálenost oka od displeje — perspektiva (menší = širší záběr) |
| `NEAR_CLIP` | 2 | hvězdy blíž k oku (v ose pohledu) se nekreslí |
| `BRIGHT_GAMMA` | 0.7 | jas = (1 − d/R)^gamma; >1 = více tmavých hvězd, hlubší pole (2.0 bylo na displeji skoro neviditelné) |
| `STAR_TINT_PROB_PCT` | 60 | % hvězd s barevným nádechem |
| `STAR_TINT_MAX` | 0.7 | max. síla nádechu 0..1 |
| `STAR_TINT_COLORS` | 3× teplá | paleta nádechu (RGB888) |
| `STAR_SIZE_PX` | 2 | strana čtverce hvězdy v px (1 = jeden pixel — na 1,8" displeji téměř neviditelný) |
| `STAR_MIN_BRIGHT` | 0.06 | hvězdy s jasem pod prahem se vůbec nekreslí |
| `GRAVITY_FILTER_ALPHA` | 0.05 | váha akcelerometru v komplementárním filtru (za snímek) |
| `GYRO_CALIB_SAMPLES` | 100 | vzorků v klidu pro bias gyra (~4 s) |
| `GYRO_STILL_DPS` | 15 | práh |ω| pro klid (surový, vč. biasu) |
| `ACCEL_STILL_G` | 0.03 | práh změny akcelerace mezi vzorky pro klid |
| `RECENTER_RATE` | 0.3 | rychlost návratu směru letu k „přímo na mě" (1/s); 0 = čistě gyro |
| `FLAT_LIMIT` | 0.3 | vodorovná složka normály displeje, pod kterou zařízení „leží" |
| `ACCEL_MAP_*`, `GYRO_MAP_*` | — | mapování os senzoru na osy displeje |
| `LCD_QSPI_HZ` | 40 MHz | takt QSPI pro přenos pixelů (80 MHz = rychlejší, ověřit obraz) |
| `STRIPE_H` | 32 | výška pruhu DMA bufferu (dělí 448, pruh < 32 KB) |
| `DISPLAY_BRIGHTNESS` | 200 | jas displeje 0–255 |
| `DEBUG_PERIOD_MS` | 1000 | perioda diagnostiky; 0 = vypnuto |

## Build

Dle CLAUDE.md, bez PSRAM (port ověřit přes `arduino-cli board list` — při vývoji byl `/dev/cu.usbmodem101`):

```sh
arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 esp32-amoled-starfield
arduino-cli upload  -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 -p /dev/cu.usbmodem101 esp32-amoled-starfield
```
