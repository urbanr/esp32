// esp32-amoled-launcher - jeden firmware s aplikacemi pisek, hvezdy a
// vodovaha, prepinani dvojklikem na displej
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz launcher.md

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"
#include "config.h"
#include "double_tap.h"

// moduly aplikaci - kazdy je vlastni prekladova jednotka (app_*.cpp),
// aby jejich static symboly a config.h nekolidovaly
bool sandBegin();  void sandLoop();  void sandEnd();
bool starBegin();  void starLoop();  void starEnd();
bool levelBegin(); void levelLoop(); void levelEnd();

struct App {
  const char *name;
  bool (*begin)();   // plna inicializace a prekresleni (jako puvodni setup())
  void (*loop)();
  void (*end)();     // uvolneni zdroju; po navratu se smi kreslit pres gfx
};

static const App apps[] = {
  { "pisek",    sandBegin,  sandLoop,  sandEnd  },
  { "hvezdy",   starBegin,  starLoop,  starEnd  },
  { "vodovaha", levelBegin, levelLoop, levelEnd },
};
static const int APP_COUNT = sizeof(apps) / sizeof(apps[0]);
static int cur = 0;

static void startApp(int i) {
  cur = i;
  // hvezdy posilaji CASET/PASET mimo knihovnu -> zneplatnit cache okna,
  // jinak by fillScreen kreslil jen do posledniho pruhu
  gfx->setRotation(0);
  gfx->fillScreen(0x0000);
  USBSerial.printf("app %s, heap %u\n", apps[i].name, ESP.getFreeHeap());
  if (!apps[i].begin()) hwHalt("app begin fail");
}

static void switchApp() {
  apps[cur].end();
  startApp((cur + 1) % APP_COUNT);
}

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");
  startApp(0);
}

void loop() {
  touchRead();
  if (doubleTapUpdate(touchDown, touchX, touchY)) switchApp();
  apps[cur].loop();
}
