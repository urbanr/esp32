# esp32-amoled-launcher

Jeden firmware pro Waveshare ESP32-S3-Touch-AMOLED-1.8, který obsahuje tři aplikace z tohoto adresáře a přepíná mezi nimi **dvojklikem na displej**, cyklicky: písek (`esp32-amoled-sand`) → hvězdy (`esp32-amoled-starfield`) → vodováha (`esp32-amoled-bubble-level`) → písek.

## Chování

- Po startu běží písek.
- Dvojklik = dvě krátká ťuknutí (každé kratší než `TAP_MAX_MS`, bez pohybu prstu nad `TAP_MOVE_PX`) do `DOUBLE_TAP_GAP_MS` od sebe a nejdál `DOUBLE_TAP_DIST_PX` od sebe. Vyhodnocuje se při uvolnění druhého ťuknutí, takže nová aplikace už žádný dotyk nedostane. Jedno ťuknutí, dlouhé držení ani tah prstem nepřepínají.
- Přepnutí = `end()` běžící aplikace, smazání displeje na černou, `begin()` další aplikace. Aplikace startuje vždy načisto (jako po zapnutí): písek s prázdnou mřížkou, hvězdy s novým polem, vodováha s bublinou uprostřed.
- V písku dvojklik smaže pár zrnek pod prstem (písek dotyk dál vidí) — záměrně přijato.
- Tlačítko BOOT patří běžící aplikaci: v písku sype, ve vodováze aretuje (aktuální náklon = nová rovina), hvězdy ho nepoužívají.

## Parametry (`config.h`)

| Makro | Výchozí | Význam |
|---|---|---|
| `TAP_MAX_MS` | 250 | max. délka stisku, aby šlo o ťuknutí |
| `TAP_MOVE_PX` | 20 | max. pohyb prstu během ťuknutí |
| `DOUBLE_TAP_GAP_MS` | 350 | max. pauza mezi uvolněním 1. a stiskem 2. ťuknutí |
| `DOUBLE_TAP_DIST_PX` | 40 | max. vzdálenost obou ťuknutí |

## Struktura

- `esp32-amoled-launcher.ino` — `setup()/loop()`, tabulka `apps[]` (jméno, `begin/loop/end`), `switchApp()`. Hardware inicializuje `../common/amoled_hw.h` (`hwInit()`), dotyk čte `../common/amoled_touch.h` (`touchRead()` každou otočku smyčky).
- `double_tap.h` — detektor dvojkliku nad polled stavem dotyku.
- `app_sand.cpp`, `app_star.cpp`, `app_level.cpp` — každý jen includuje modul aplikace z jejího původního adresáře (`../esp32-amoled-sand/sand_app.h` atd.). Každá aplikace je tak vlastní překladová jednotka: její `static` symboly i její `config.h` nekolidují s ostatními a nic se nepřejmenovává.
- Modul aplikace (`<app>_app.h`) poskytuje `bool xxxBegin()` (plná inicializace a překreslení), `void xxxLoop()` a `void xxxEnd()` (po něm musí jít kreslit přes `gfx`: hvězdy dokončí DMA frontu, vodováha uvolní canvasy). Stejný modul používá i samostatný sketch aplikace.

## Hardware

- SPI sběrnice displeje se inicializuje jednou (`hwInit()`, `max_transfer_sz` podle pruhů hvězd); knihovna Arduino_GFX na ni jen přidá své zařízení (`GFX_SKIP_DATABUS_UNDERLAYING_BEGIN`, `is_shared_interface`). DMA zařízení hvězd se přidá při prvním startu hvězd a zůstává.
- Po hvězdách launcher volá `gfx->setRotation(0)`: hvězdy posílají CASET/PASET mimo knihovnu a knihovna by se stejným oknem v cache tyto příkazy vynechala.
- Launcher nesmí kreslit přes `gfx`, dokud běží hvězdy (sdílený CS pin).
- Bez PSRAM: ~165 KB statické RAM (z toho hvězdy ~120 KB) + canvasy vodováhy 40 KB jen za jejího běhu. Při každém přepnutí se do logu píše volný heap — po prvním cyklu musí být stabilní.

## Build

Stejné FQBN a port jako ostatní aplikace (viz `CLAUDE.md`):

```sh
arduino-cli compile -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 esp32-amoled-launcher
arduino-cli upload  -b esp32:esp32:waveshare_esp32_s3_touch_amoled_18:CDCOnBoot=cdc,UploadSpeed=115200 -p /dev/cu.usbmodem1101 esp32-amoled-launcher
```
