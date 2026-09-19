# sdlauncher — seznam aplikací z SD karty

Launcher pro Waveshare ESP32-S3-Touch-AMOLED-2.41. Na rozdíl od launcheru na 1.8 nejsou aplikace v jednom firmwaru, ale jako samostatné binárky na SD kartě.

## Jak to funguje

1. Launcher je nahraný v oddílu `ota_0` (jediný sketch, který se do desky nahrává přes arduino-cli).
2. Při startu otevře SD kartu (SPI, piny z `../common/pin_config.h`) a vypíše `/apps/*.bin`. Název souboru bez přípony je to, co se ukáže v seznamu.
3. Ťuknutí na položku zkopíruje binárku do oddílu `ota_1` (`esp_ota_begin/write/end`) s ukazatelem postupu, přepne na něj boot a restartuje se.
4. Aplikace si hned po startu v `hwInit()` přepne bootovací oddíl zpět na launcher (`../common/amoled_boot.h`) — **jakýkoli restart tedy vrátí seznam**.

Spouštět kód přímo z karty nejde: ESP32 vykonává kód jen z namapované flash. Proto se binárka kopíruje (~0,4–0,7 MB, jednotky sekund).

## Karta

- Formát FAT32, binárky v adresáři `/apps`, nejvýše 12 položek (`MAX_APPS`).
- Kartu lze zasunout i za běhu — launcher ji zkouší otevřít každou 1,5 s a seznam se sám objeví.
- Binárky vyrobí `../build-apps.sh` do `../sd-apps/`.

## Chování při chybách

Hláška ve spodním pruhu: chybějící karta, prázdný `/apps`, binárka větší než oddíl, poškozený obraz (kontroluje `esp_ota_end`), selhání čtení nebo zápisu. Po chybě se seznam vrátí a dá se zkusit jiná aplikace.

Spodní řádek ukazuje souřadnice posledního ťuknutí — slouží ke kontrole, že dotyk sedí se seznamem.
