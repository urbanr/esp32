# CLAUDE.md

Adresář s ESP32 projekty. Každé zařízení má svůj podadresář a svou kapitolu níže. Nové zařízení = nová kapitola.

## Přehled adresářů

| Adresář | Zařízení | Kapitola |
|---|---|---|
| `ESP32-S3-Touch-AMOLED-1.8/` | Waveshare ESP32-S3-Touch-AMOLED-1.8 (aplikace: `esp32-amoled-sand/` — spec `sand.md`, `esp32-amoled-starfield/` — spec `starfield.md`, `esp32-amoled-bubble-level/` — spec `bubble-level.md`, `esp32-amoled-launcher/` — spec `launcher.md`, všechny tři v jednom firmwaru; `esp32-amoled-ship-navigator/` — spec `ship-navigator.md`, zatím samostatně; `esp32-rat-jumper/` — spec `rat-jumper.md`, displej na šířku, zatím samostatně; `esp32-rat-zombies/` — spec `rat-zombies.md`, grafika v `grafika-v3/` generovaná do `zomb_gfx.h` skriptem `gen_gfx.py`, displej na šířku, zatím samostatně; sdílený kód `common/` (hardware, dotyk, zvuk ES8311); dále `ESP32-S3-Touch-AMOLED-1.8-test/`, `motoriste-kokoti/`) | níže |
| `ESP32-S3-Touch-AMOLED-2.4/` | Waveshare ESP32-S3-Touch-AMOLED-2.41 (kopie stromu 1.8; přizpůsoben zatím jen `esp32-rat-jumper/` a `common/`, ostatní aplikace se pro tuto desku nepřekládají) | níže |
| `128display-test/` | zatím bez kapitoly | — |

---

## ESP32-S3-Touch-AMOLED-1.8

Platí pro celý adresář `ESP32-S3-Touch-AMOLED-1.8/` včetně všech podprojektů.

### Hardware

- ESP32-S3 s AMOLED displejem **SH8601** na QSPI sběrnici, řízený knihovnou **Arduino_GFX_Library**.
- Paměť: **512 KB interní SRAM** (reálně použitelný heap ~300–350 KB), **8 MB oktální PSRAM** (čip ESP32-S3R8), **16 MB flash**. PSRAM je v CLI buildech defaultně vypnutá — viz sekce PSRAM níže.
- Displej je napájený přes I2C expander **XCA9554** (adresa `0x20`). Před inicializací displeje je nutné příslušné piny expanderu nastavit jako OUTPUT a přepnout na HIGH, s krátkým delay pro reset.
- Logování přes **HWCDC USBSerial**. Piny jsou v `pin_config.h`.

### Inicializace displeje

```cpp
Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);
```

Aplikace písek / hvězdy / vodováha a launcher tuto inicializaci nedělají samy — používají `hwInit()` z `common/amoled_hw.h` (viz „Sdílený kód a launcher" níže), který navíc sám inicializuje SPI sběrnici s větší max. transakcí a knihovně předá `GFX_SKIP_DATABUS_UNDERLAYING_BEGIN`.

### Sdílený kód a launcher

- `common/amoled_hw.h` — **jediná** inicializace hardwaru (`hwInit()`: USBSerial + `setTxTimeoutMs(0)`, I2C recovery, XCA9554, `spi_bus_initialize` s `max_transfer_sz = AMOLED_SPI_MAX_TRANSFER`, `gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN)`, černý displej, jas). Sběrnice se tvoří s `is_shared_interface = true`, aby ji knihovna nedržela trvale a DMA zařízení hvězd se k ní dostalo. Obsahuje **definice** objektů (`USBSerial`, `gfx`, …) — includovat jen z hlavního `.ino`.
- `common/amoled_touch.h` — dotyk FT3168 (`touchBegin()`, `touchRead()` každou otočku smyčky → `touchDown`, `touchX`, `touchY`). Také definice, také jen z hlavního `.ino`.
- `common/amoled_app.h` — rozhraní pro moduly aplikací (`extern gfx`, `USBSerial`, stav dotyku, makra `AMOLED_BRIGHTNESS`, `AMOLED_SPI_MAX_TRANSFER`). Moduly includují jen tento soubor.
- **Modul aplikace** `<app>_app.h` v adresáři aplikace: ne-`static` funkce `bool xxxBegin()` (plná inicializace + celé překreslení, vynulování `lastUs/timeAcc`), `void xxxLoop()`, `void xxxEnd()` (po něm musí jít kreslit přes `gfx`: hvězdy `waitPending(0)`, vodováha uvolní canvasy). Vše ostatní v modulu zůstává `static`. Samostatný sketch aplikace je tenká obálka `hwInit(); xxxBegin();` / `xxxLoop();`.
- **Launcher** `esp32-amoled-launcher/`: jedna aplikace = jeden `app_*.cpp`, který jen includuje `../<app>/<app>_app.h`. Každá aplikace je tak vlastní překladová jednotka → její `static` symboly (`qmi`, `inputRead`, `renderInit`, …) ani její `config.h` nekolidují s ostatními. Nová aplikace = modul + řádek v tabulce `apps[]`. Launcher nesmí kreslit přes `gfx`, dokud běží hvězdy (sdílený CS pin); před kreslením po hvězdách volá `gfx->setRotation(0)` (invalidace cache okna CASET/PASET v knihovně).
- Include přes `../` (`"../common/amoled_hw.h"`, `"../esp32-amoled-sand/sand_app.h"`) funguje v arduino-cli i Arduino IDE (core přidává `-I{build.source.path}`); vnořený `#include "config.h"` se hledá nejdřív v adresáři hlavičky, takže každá aplikace dostane svůj. V IDE se soubory mimo sketch neukazují v záložkách.

### Pravidla pro neblikající (flicker-free) vykreslování

- **Nikdy** nepřekresluj dynamický obsah přímo na displej sekvencí „smaž oblast (`fillRect`) → nakresli nový obsah". Displej mezi oběma kroky zobrazí prázdnou plochu, což způsobuje problikávání.
- Používej **Arduino_Canvas** jako offscreen buffer. Vytvoř canvas jen pro oblast, která se dynamicky mění (šetří RAM — RGB565 = 2 bajty na pixel), s offsetem určujícím jeho pozici na displeji: `new Arduino_Canvas(sirka, vyska, gfx, offset_x, offset_y)`. Veškeré kreslení dynamického obsahu (fill, text, tvary) prováděj do canvasu a hotový snímek pošli na displej jediným voláním `canvas->flush()`. Displej tak nikdy neuvidí mezistav.
- **KRITICKÉ — pořadí a způsob inicializace:** `canvas->begin()` bez parametru volá i `begin()` podkladového výstupu. Pokud už proběhl `gfx->begin()`, dojde k druhé inicializaci QSPI/SPI sběrnice, ESP-IDF vrátí `ESP_ERR_INVALID_STATE`, `ESP_ERROR_CHECK` shodí čip a zařízení skončí v nekonečné reboot smyčce (projevuje se černou obrazovkou a hláškou `spi_bus_initialize: SPI bus already initialized` na sériovém monitoru). Správně tedy vždy: nejdřív `gfx->begin()`, potom `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)`, a zkontroluj návratovou hodnotu — `false` znamená, že selhala alokace bufferu (málo RAM).
- Souřadnice uvnitř canvasu jsou relativní k canvasu, ne k displeji. Canvas s offsetem y=240 znamená, že bod y=250 na displeji odpovídá y=10 v canvasu.
- Statický obsah (nadpisy, rámečky, pozadí, které se nemění) kresli jednou v `setup()` přímo přes `gfx`. Přes canvas veď jen to, co se opakovaně překresluje.
- Pomocné kreslicí funkce piš tak, aby braly cíl jako parametr `Arduino_GFX *dst` — pak fungují shodně pro `gfx` i canvas (obě třídy dědí ze stejného základu).
- Pokud je dynamických oblastí víc a jsou daleko od sebe, je úspornější více menších canvasů než jeden velký; pokud pokrývají většinu displeje, zvaž jeden fullscreen canvas (u velkých rozlišení vyžaduje PSRAM — viz poznámka o PSRAM níže).

### Známé pasti

- **Zaseklá I2C sběrnice po reflashi:** soft reset (každý upload) může přerušit I2C transakci uprostřed a některý slave (FT3168/QMI8658/XCA9554) pak drží SDA — veškerá I2C komunikace selhává (projev: „XCA9554 init fail", černý displej), dokud se zařízení neodpojí od napájení. Řešení: před `Wire.begin()` provést recovery — 9 pulzů na SCL + STOP condition (viz `i2cBusRecover()` v `common/amoled_hw.h`) a inicializaci expanderu pár× zopakovat.
- **Cache adresového okna v Arduino_GFX:** `Arduino_SH8601::writeAddrWindow` pošle CASET/PASET jen při změně okna. Kdo posílá tyto příkazy panelu mimo knihovnu (hvězdy přes vlastní DMA), musí před dalším kreslením přes `gfx` zavolat `gfx->setRotation(0)` — jinak knihovna kreslí do cizího okna (projev: `fillScreen` smaže jen pruh 32 řádků).
- **Hlavičky s definicemi** (`common/amoled_hw.h`, `common/amoled_touch.h`) smí includovat jen hlavní `.ino`; z modulu aplikace by vznikla dvojí definice (chyba linkeru v launcheru).
- BOOT tlačítko (GPIO0) je jediné uživatelské tlačítko vyvedené jako běžný GPIO (`INPUT_PULLUP`, stisk = LOW).
- **Blokující USBSerial (HWCDC):** když na počítači nikdo sériový port nečte, `USBSerial.print/printf` čeká na vyprázdnění USB FIFO s timeoutem — každý výpis zastaví smyčku na desítky až stovky ms. Animace pak vypadá jako ~1 fps a „opraví se", jakmile se otevře sériový monitor (matoucí!). Řešení: hned po `USBSerial.begin()` zavolat `USBSerial.setTxTimeoutMs(0)` — nečtený výstup se zahodí, smyčka neblokuje (zjištěno v esp32-amoled-starfield).

### Výkon a fps (zkušenosti z CRT filtrů rakety a krysy)

- **Měřit, ne odhadovat.** Debug výpis fps počítej z neoříznutého času snímku (`micros()` před a po smyčce). Když se `dt` pro fyziku ořezává (např. na 50 ms), ořezaný součet dá stále „20 fps", i když je snímek delší. Rozděl měření na kreslení scény / skládání pruhů / čekání na DMA; u krysy se ukázalo, že 49 z 67 ms byla příprava sloupců, ne pixely.
- **Strop je DMA:** celý displej 368×448 RGB565 přes QSPI 40 MHz trvá ~15–16 ms → max ~60 fps. Pruhy 368×32 s ping-pong buffery a `waitPending(1)` počítají další pruh během přenosu; cokoli nad 16 ms výpočtu na snímek už fps snižuje.
- **Cena za pixel je v paměťových přístupech**, ne v aritmetice: na 240 MHz vyšlo ~5 cyklů na load/store. Násobení v pixelové smyčce nahradit tabulkami; ideál je **jedno čtení předpočítané tabulky na pixel** (krysa: `fastLut[vinětace][fáze masky][fáze scanline][index palety]` → hotový RGB565 s prohozenými bajty, 36 KB). Míchání dvou hodnot (prosvit sousedního řádku, prolnutí se sousedním sloupcem) násobí cenu 2–3× (16 fps vs. 34 fps).
- **Nepočítat totéž vícekrát:** u otočeného obrazu odpovídá jeden logický sloupec třem fyzickým řádkům; příprava sloupce se dělala 448× místo 150×. Posuvné okno předpočítaných sloupců (předchozí/aktuální/další) to řeší.
- **Zpracovávat po skupinách sdílejících data** (trojice fyzických px jednoho logického bodu): ušetří opakované indexové load-y.
- **`-O2` místo výchozího `-Os`**: soubor `build_opt.h` ve sketchi s řádkem `-O2` (core ho přidá ke všem překladům sketche i knihoven). Kreslení scény zrychlilo ~1,7×, pixelová smyčka ~1,3×; flash +4 %.
- **Indexovaný canvas** (`Arduino_Canvas_Indexed`, `setDirectUseColorIndex(true)`): 8 bit na bod, index palety se předává přímo jako „barva" — bez hledání v paletě při každém zápisu.
- Statické tabulky a buffery drž v interní RAM (žádná PSRAM), DMA buffery musí být interní tak jako tak.

### Build a upload (arduino-cli)

Deska je z esp32 core (`esp32:esp32`, verze 3.3.11). Výchozí FQBN pro CLI buildy:

```
esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200
```

- `CDCOnBoot=cdc` (USB CDC On Boot: Enabled) je **nutné** kvůli logování přes HWCDC USBSerial — vždy uvádět.
- `UploadSpeed=115200` — pomalejší, ale spolehlivé (default desky je 921600).
- Ostatní volby desky zůstávají na defaultech (USB Mode: Hardware CDC and JTAG, Flash Mode: QIO 80MHz, Partition Scheme: 16M Flash 3MB APP/9.9MB FATFS, CPU 240 MHz).

Port: `/dev/cu.usbmodem1101`

```sh
arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 <sketch_dir>
arduino-cli upload  -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 -p /dev/cu.usbmodem1101 <sketch_dir>
```

#### PSRAM

- Výchozí CLI build je **bez PSRAM** (`PSRAM=disabled` je default desky). Záměr: nejdřív vyzkoušet řešení bez PSRAM, teprve pak s ní.
- Bez PSRAM musí canvasy pokrývat jen dynamické oblasti (interní RAM je omezená). Fullscreen canvas dává smysl až s PSRAM.
- Varianta s PSRAM pro testování: přidat do FQBN `,PSRAM=enabled`.
- Pozor: v Arduino IDE je aktuálně PSRAM **zapnutá** — build z IDE se tedy může lišit od výchozího CLI buildu, dokud se nastavení nesjednotí. Není to chyba, jen možný zdroj rozdílů v chování.

#### Sdílení s Arduino IDE

- arduino-cli a Arduino IDE sdílejí stejné adresáře: `~/Library/Arduino15` (jádra) a `~/Documents/Arduino/libraries` (knihovny). Je to jedna instalace — změna verze knihovny v IDE se projeví i v CLI buildu a naopak.
- **Verze knihoven určuje uživatel** (experimentuje s nimi v IDE Library Manageru) — neměnit je bez dohody.
- Pro arduino-cli nezakládat vlastní config s jinými `directories.user` / `directories.data` — rozjely by se světy CLI a IDE.
- Volby desky (Tools menu vs. FQBN) se mezi IDE a CLI nesdílejí; kanonické hodnoty pro CLI jsou zapsané výše.

---

## ESP32-S3-Touch-AMOLED-2.4

Adresář `ESP32-S3-Touch-AMOLED-2.4/` je kopie stromu 1.8 pro desku **Waveshare ESP32-S3-Touch-AMOLED-2.41**. Platí pravidla kapitoly 1.8 (flicker-free kreslení, výkon a fps, sdílení s Arduino IDE), liší se hardware níže. Přizpůsoben je zatím **jen `esp32-rat-jumper/` a `common/`**; ostatní zkopírované aplikace se pro tuto desku nepřekládají (čekají na úpravu).

### Rozdíly proti 1.8

| | 1.8 | 2.41 |
|---|---|---|
| Řadič displeje | SH8601 | **RM690B0** (paměť širší než panel → offset sloupců 16) |
| Rozlišení | 368×448 | **450×600** |
| QSPI (CS, SCLK, D0–D3) | 12, 11, 4, 5, 6, 7 | **9, 10, 11, 12, 13, 14**, RESET **21** |
| I2C (SDA, SCL) | 15, 14 | **47, 48** |
| Dotyk | FT3168, INT na GPIO21 | **FT6336** (stejné registry, ovladač `Arduino_FT3x68`), RST **3**, INT jen na expanderu → pollování |
| Zvuk | kodek ES8311 | **není** — `common/amoled_audio.h` je záslepka, `audioBegin()` vrací false |
| SD karta | SDMMC 1 bit | **SPI** (CS 2, SCLK 4, MOSI 5, MISO 6) na vlastní sběrnici HSPI |
| Napájení | expander XCA9554 povoluje displej | **BAT_PWR = GPIO16 HIGH** a expander 0x20 přes přímý zápis registrů: `0x03 = 0x00` (vše výstup), `0x01 = 0xFF` (vše HIGH) |

- Piny jsou ve **vlastním `common/pin_config.h`** (1.8 bere stejnojmenný soubor z knihovny `Mylibrary`). Aplikace ho includují jako `"../common/pin_config.h"`, aby se nechytil ten knihovní.
- Vlastní DMA kreslení (`rat_crt.h`) musí k adresnímu oknu přičíst `LCD_X_OFF` (16), jinak je obraz posunutý.

### Známé pasti

- **Panel zůstane tmavý, dokud nejsou EXIO expanderu nastavené jako výstupy v HIGH.** Ověřeno pokusem (sketch, který po restartech cykloval varianty): s piny ponechanými jako vstupy i se všemi v LOW je displej černý, `0x01 = 0xFF` + `0x03 = 0x00` ho rozsvítí. Knihovní příklad `PDQgraphicstest` pro tuto desku expander vůbec nenastavuje.
- **Vypnutý panel a černý obraz se nepoznají.** Aplikace po startu maže displej na černo, takže špatná inicializace vypadá stejně jako špatné kreslení — na rozlišení toho slouží sketch, který kreslí barevné pruhy a číslo varianty.
- Pro diagnostiku se hodí I2C sken v `hwInit()`; zdravá deska hlásí `0x20 0x38 0x51 0x6B 0x7E`. Boot log se nedá zachytit (`setTxTimeoutMs(0)` + re-enumerace USB), proto výpisy opakovat ve smyčce.

### Build a upload

```
esp32:esp32:waveshare_esp32_s3_touch_amoled_241:CDCOnBoot=cdc,UploadSpeed=115200
```

Port `/dev/cu.usbmodem1101`, jinak stejné jako u 1.8.

### rat-jumper na 2.41

- Hrací plocha **200×150 bodů** (1.8: 150×123) při stejné velikosti bodu 3×3 px — větší výhled dopředu i nad krysou.
- `FLOOR_Y`, `LANE1_Y`, `LANE2_Y` v `config.h` jsou nově odvozené od `LH` (chodník 25 bodů ode dna), takže rozestupy skoků zůstávají stejné jako na 1.8.
- `STRIPE_H = 30` (musí dělit 600).
- Naměřeno **20,8 fps** (1.8: 34 fps) — plocha má 1,64× víc pixelů, čas jde skoro celý do skládání pruhů, čekání na DMA je zanedbatelné (~0,2 ms).
