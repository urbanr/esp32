// Sluzebni test: prezije panel softwarovy restart? Pouziva stejny
// hwInit() jako aplikace. Po kazdem restartu prebarvi displej a cislo
// kola vypise na seriovou linku - kdyz obraz po restartu zcerna, je
// chyba v probouzeni panelu, ne v aplikacich.
#include "../common/amoled_hw.h"

RTC_NOINIT_ATTR uint32_t magic;
RTC_NOINIT_ATTR int round_;

static const uint16_t barvy[] = { RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW, RGB565_MAGENTA };
static const char *jmena[] = { "CERVENA", "ZELENA", "MODRA", "ZLUTA", "FIALOVA" };

void setup() {
  hwInit();
  if (magic != 0xC0FFEE03) { magic = 0xC0FFEE03; round_ = 0; }
  const int i = round_ % 5;
  gfx->fillScreen(barvy[i]);
  gfx->setTextSize(4);
  gfx->setTextColor(RGB565_BLACK);
  gfx->setCursor(40, 260);
  gfx->printf("%d %s", round_, jmena[i]);
}

void loop() {
  for (int t = 0; t < 5; t++) {
    USBSerial.printf("kolo %d (%s) bezi, expander %s\n", round_, jmena[round_ % 5], exioOk ? "ok" : "CHYBA");
    delay(1000);
  }
  round_++;
  esp_restart();
}
