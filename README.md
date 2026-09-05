# esp32

Pokusy a hračky pro ESP32, psané v Arduino C++ (arduino-cli / Arduino IDE, esp32 core 3.x). Každé zařízení má svůj adresář; pravidla pro build a poznámky k hardwaru jsou v [`CLAUDE.md`](CLAUDE.md), každá větší aplikace má vlastní specifikaci `*.md`.

## Waveshare ESP32-S3-Touch-AMOLED-1.8

Deska s AMOLED displejem SH8601 368×448 px (QSPI), dotykem FT3168, IMU QMI8658, kodekem ES8311 a reproduktorem. Adresář [`ESP32-S3-Touch-AMOLED-1.8/`](ESP32-S3-Touch-AMOLED-1.8/). Aplikace sdílejí inicializaci hardwaru a čtení dotyku v [`common/`](ESP32-S3-Touch-AMOLED-1.8/common/) a jsou napsané jako moduly `begin/loop/end`, takže běží samostatně i v launcheru.

| Aplikace | Co dělá | Specifikace |
|---|---|---|
| [`esp32-amoled-sand`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-sand/) | Padající písek řízený náklonem (IMU). BOOT sype, dotyk maže. Diferenciální překreslování bez framebufferu. | [`sand.md`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-sand/sand.md) |
| [`esp32-amoled-starfield`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-starfield/) | Průlet hvězdným polem; zařízení je kamera (akcelerometr + gyroskop). Vlastní DMA renderer po pruzích, ~50 fps. | [`starfield.md`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-starfield/starfield.md) |
| [`esp32-amoled-bubble-level`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-bubble-level/) | Kruhová vodováha se zelenou kapalinou, bublinou a číselným náklonem. BOOT aretuje aktuální rovinu. | [`bubble-level.md`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-bubble-level/bubble-level.md) |
| [`esp32-amoled-launcher`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-launcher/) | Písek, hvězdy a vodováha v jednom firmwaru, přepínání dvojklikem na displej. | [`launcher.md`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-launcher/launcher.md) |
| [`esp32-amoled-ship-navigator`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-ship-navigator/) | Dětská palubní deska rakety ve stylu zeleného CRT z 80. let: letové údaje, navigace mezi planetami, zásoby, ladění CRT filtru; zvuky přes reproduktor. | [`ship-navigator.md`](ESP32-S3-Touch-AMOLED-1.8/esp32-amoled-ship-navigator/ship-navigator.md) |
| `ESP32-S3-Touch-AMOLED-1.8-test` | Testovací sketch desky: displej, dotyk, bitmapa přes celý displej. | — |
| `motoriste-kokoti` | Ukázka s bitmapou 368×448 přes celý displej. | — |

### Raketa (ship-navigator)

Let trvá minutu a řídí se tlačítkem BOOT (start, pauza, nový let), obrazovky se přepínají swipem. Vše se kreslí do bufferu s hrubou mřížkou (výchozí 4×4 px na bod) a při přenosu na displej se aplikuje filtr napodobující starý monitor: dosvit fosforu, bloom, scanlines, vinětace, blikání, plovoucí pruh, jiskry. Čtvrtá obrazovka parametry vypisuje, ťuknutí je losuje a tah nahoru/dolů mění mřížku (2×2 až 6×6).

<p>
<img src="docs/img/ship-navigator-letove-udaje.png" width="220" alt="Letové údaje">
<img src="docs/img/ship-navigator-navigace.png" width="220" alt="Navigace mezi planetami">
<img src="docs/img/ship-navigator-zasoby.png" width="220" alt="Zásoby">
<img src="docs/img/ship-navigator-crt.png" width="220" alt="Parametry CRT filtru">
</p>

Zleva: letové údaje, navigace (planety mají náhodná jména, třeba BUBLANINA nebo LOCHTA), zásoby, parametry CRT filtru.

## 1.5" RGB OLED SSD1351 (128×128, SPI) na ESP32

Adresář [`128display-test/`](128display-test/), knihovny Adafruit_GFX + Adafruit_SSD1351.

| Sketch | Co dělá |
|---|---|
| `displayOnOffTest` | Počítadlo s tlačítkem, překreslování jen části displeje přes malý buffer (128×32). |
| `finger` | Černý displej; jednou v každém 30 s okně se na 2 s v náhodný okamžik ukáže reliéfní obrázek (128×128 RGB565 v PROGMEM, z PNG přes emboss filtr). |
| `fingerRelief` | Kombinace obou: počítadlo a občasný reliéfní obrázek. |

## Build

```sh
arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 <sketch_dir>
arduino-cli upload  -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 -p /dev/cu.usbmodem1101 <sketch_dir>
```

Podrobnosti (PSRAM, sdílení s Arduino IDE, známé pasti jako zaseklá I2C po reflashi nebo blokující USBSerial) jsou v [`CLAUDE.md`](CLAUDE.md).

## Licence

MIT, viz [`LICENSE`](LICENSE).
