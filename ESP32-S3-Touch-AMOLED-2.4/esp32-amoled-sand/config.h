#pragma once

// ===== mrizka =====
#define SAND_SCALE            12    // display pixelu na zrnko; pri nedelitelnem rozliseni se hraci plocha vycentruje
#define TICK_HZ               60    // frekvence simulace

// ===== fyzika (hodnoty dle JS predlohy) =====
#define SLIDE_SURFACE_PROB    80    // % - bocni skluz po povrchu (padajici zrnko, calm == 0)
#define SLIDE_SETTLED_PROB    15    // % - bocni skluz ustalene kupy (calm > 0)
#define CALM_TICKS_TO_FREEZE  30    // klidnych ticku do zamrznuti zrnka
#define STICKINESS_PROB       25    // % - docasna lepivost (sance drzet, per tick)
#define MAX_HOLD_TICKS        15    // maximalni delka drzeni lepivosti (ticky)

// ===== mazani dotykem =====
#define TOUCH_ERASE_PX_PER_S  300   // zrnek za sekundu, rozsah 10-1000
#define ERASE_ZONE_PX         30    // strana mazaci zony (display px)

// ===== sypani =====
#define POUR_BUTTON_PIN       0     // BOOT tlacitko - sype se jen pri drzeni
#define EMIT_MAX_PX_PER_S     50    // max. rychlost sypani (displej svisle)
#define EMIT_ZONE_BASE_PX     20    // zakladni strana sypaci zony (display px)
#define SHAKE_ZONE_GAIN       3.0f  // zesileni rozsireni zony trepanim

// ===== gravitace =====
#define TILT_DEADZONE         0.08f // g - pod touto slozkou v rovine displeje simulace stoji
#define WAKE_ANGLE_DEG        20.0f // zmena smeru gravitace, ktera probudi vsechna zrnka

// Mapovani os akcelerometru (v g) na osy displeje: +X doprava, +Y dolu.
// Pokud pisek pada spatnym smerem, uprav znamenka/prohod osy zde.
#define ACCEL_MAP_GX(ax, ay, az)  (-(ay))
#define ACCEL_MAP_GY(ax, ay, az)  (ax)

// ===== piskova paleta =====
#define PALETTE_CONTRAST_PCT  60    // 0-100
#define MAX_DARKEN            0.75f // ztmaveni nejtmavsiho odstinu pri 100% kontrastu
#define SAND_BASE_R           243
#define SAND_BASE_G           222
#define SAND_BASE_B           158

// ===== vzhled =====
#define BG_R                  0
#define BG_G                  0
#define BG_B                  0
