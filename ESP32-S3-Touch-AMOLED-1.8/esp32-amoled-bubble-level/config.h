#pragma once

// ===== mapovani naklonu =====
#define TILT_FULL_SCALE_DEG   20.0f // naklon, pri kterem je bublina na kraji
#define BUBBLE_CURVE_EXP      0.5f  // <1 = citlive u stredu, 1.0 = linearni
#define TILT_DEADZONE_G       0.01f // pod touto slozkou v rovine displeje je cil stred

// ===== pohyb bubliny =====
#define MOTION_SMOOTH         0
#define MOTION_PHYSICS        1
#define BUBBLE_MOTION_MODE    MOTION_SMOOTH
#define SMOOTH_ALPHA          0.15f // dohlazeni v rezimu SMOOTH (za tick)
#define SPRING_K              40.0f // tuhost pruziny v rezimu PHYSICS (/s^2)
#define DAMPING               0.90f // tlumeni rychlosti v rezimu PHYSICS (za tick)
#define TICK_HZ               60    // frekvence smycky

// ===== geometrie (display px) =====
#define LIQUID_RADIUS_PX      184   // kruh kapaliny pres celou sirku displeje
#define BUBBLE_RADIUS_PX      46    // polomer bubliny (bez okraje)
#define BUBBLE_EDGE_THICKNESS_PX 3  // tmavsi okraj bubliny
#define TARGET_RADIUS_PX      62    // referencni kruznice
#define TARGET_THICKNESS_PX   3
#define BUBBLE_R_MAX_PX       128   // max. vychylka stredu bubliny
#define BUBBLE_CANVAS_PX      128   // strana canvasu bubliny

// ===== barvy (RGB888) =====
#define CASE_COLOR            0, 0, 0        // pouzdro mimo kruh kapaliny
#define LIQUID_CENTER_COLOR   178, 214, 90   // kapalina uprostred
#define LIQUID_EDGE_COLOR     105, 150, 45   // kapalina u okraje
#define GLASS_RIM_DARK        60, 90, 30     // tmavy prstenec u okraje skla
#define GLASS_RIM_LIGHT       220, 235, 190  // svetly oblouk odlesku skla
#define BUBBLE_FILL_COLOR     198, 226, 120  // vypln bubliny
#define BUBBLE_EDGE_COLOR     95, 140, 45    // tmavsi okraj bubliny
#define BUBBLE_HIGHLIGHT_COLOR 240, 248, 215 // lem a srpek odlesku
#define TARGET_COLOR          0, 0, 0        // cerna referencni kruznice
#define TILT_TEXT_COLOR       140, 160, 120  // text naklonu na cernem pouzdru

// ===== text naklonu =====
#define TILT_TEXT_PERIOD_MS   200   // min. perioda prekresleni textu

// Mapovani os akcelerometru (v g) na osy displeje: +X doprava, +Y dolu
// (stejne jako v esp32-amoled-sand). Znamenka pro cislny udaj naklonu:
// kladne X = zvednuty pravy okraj, kladne Y = zvednuty horni okraj.
#define ACCEL_MAP_GX(ax, ay, az)  (-(ay))
#define ACCEL_MAP_GY(ax, ay, az)  (ax)
#define TILT_X_SIGN           (-1)
#define TILT_Y_SIGN           (+1)

#define DISPLAY_BRIGHTNESS    200   // 0-255
