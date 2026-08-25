# CLAUDE.md

Adresář s ESP32 projekty. Každé zařízení má svůj podadresář a svou kapitolu níže. Nové zařízení = nová kapitola.

## Přehled adresářů

| Adresář | Zařízení | Kapitola |
|---|---|---|
| `ESP32-S3-Touch-AMOLED-1.8/` | Waveshare ESP32-S3-Touch-AMOLED-1.8 (podprojekty: `ESP32-S3-Touch-AMOLED-1.8-test/`, `motoriste-kokoti/`, `esp32-amoled-starfield/` — spec `starfield.md`) | níže |
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

### Pravidla pro neblikající (flicker-free) vykreslování

- **Nikdy** nepřekresluj dynamický obsah přímo na displej sekvencí „smaž oblast (`fillRect`) → nakresli nový obsah". Displej mezi oběma kroky zobrazí prázdnou plochu, což způsobuje problikávání.
- Používej **Arduino_Canvas** jako offscreen buffer. Vytvoř canvas jen pro oblast, která se dynamicky mění (šetří RAM — RGB565 = 2 bajty na pixel), s offsetem určujícím jeho pozici na displeji: `new Arduino_Canvas(sirka, vyska, gfx, offset_x, offset_y)`. Veškeré kreslení dynamického obsahu (fill, text, tvary) prováděj do canvasu a hotový snímek pošli na displej jediným voláním `canvas->flush()`. Displej tak nikdy neuvidí mezistav.
- **KRITICKÉ — pořadí a způsob inicializace:** `canvas->begin()` bez parametru volá i `begin()` podkladového výstupu. Pokud už proběhl `gfx->begin()`, dojde k druhé inicializaci QSPI/SPI sběrnice, ESP-IDF vrátí `ESP_ERR_INVALID_STATE`, `ESP_ERROR_CHECK` shodí čip a zařízení skončí v nekonečné reboot smyčce (projevuje se černou obrazovkou a hláškou `spi_bus_initialize: SPI bus already initialized` na sériovém monitoru). Správně tedy vždy: nejdřív `gfx->begin()`, potom `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)`, a zkontroluj návratovou hodnotu — `false` znamená, že selhala alokace bufferu (málo RAM).
- Souřadnice uvnitř canvasu jsou relativní k canvasu, ne k displeji. Canvas s offsetem y=240 znamená, že bod y=250 na displeji odpovídá y=10 v canvasu.
- Statický obsah (nadpisy, rámečky, pozadí, které se nemění) kresli jednou v `setup()` přímo přes `gfx`. Přes canvas veď jen to, co se opakovaně překresluje.
- Pomocné kreslicí funkce piš tak, aby braly cíl jako parametr `Arduino_GFX *dst` — pak fungují shodně pro `gfx` i canvas (obě třídy dědí ze stejného základu).
- Pokud je dynamických oblastí víc a jsou daleko od sebe, je úspornější více menších canvasů než jeden velký; pokud pokrývají většinu displeje, zvaž jeden fullscreen canvas (u velkých rozlišení vyžaduje PSRAM — viz poznámka o PSRAM níže).

### Známé pasti

- **Zaseklá I2C sběrnice po reflashi:** soft reset (každý upload) může přerušit I2C transakci uprostřed a některý slave (FT3168/QMI8658/XCA9554) pak drží SDA — veškerá I2C komunikace selhává (projev: „XCA9554 init fail", černý displej), dokud se zařízení neodpojí od napájení. Řešení: před `Wire.begin()` provést recovery — 9 pulzů na SCL + STOP condition (viz `i2cBusRecover()` v esp32-amoled-sand) a inicializaci expanderu pár× zopakovat.
- BOOT tlačítko (GPIO0) je jediné uživatelské tlačítko vyvedené jako běžný GPIO (`INPUT_PULLUP`, stisk = LOW).
- **Blokující USBSerial (HWCDC):** když na počítači nikdo sériový port nečte, `USBSerial.print/printf` čeká na vyprázdnění USB FIFO s timeoutem — každý výpis zastaví smyčku na desítky až stovky ms. Animace pak vypadá jako ~1 fps a „opraví se", jakmile se otevře sériový monitor (matoucí!). Řešení: hned po `USBSerial.begin()` zavolat `USBSerial.setTxTimeoutMs(0)` — nečtený výstup se zahodí, smyčka neblokuje (zjištěno v esp32-amoled-starfield).

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
