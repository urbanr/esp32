# esp32-rat-zombies — Krysy a zombíci, specifikace

Dětská jezdící hra pro Waveshare ESP32-S3-Touch-AMOLED-1.8, displej **na šířku** (BOOT dole), stejná logická mřížka a způsob vykreslování jako Krysa skokan (`../esp32-rat-jumper/`): **150 × 123 bodů**, 1 bod = 3×3 px, 8bitový indexovaný canvas, přenos po pruzích vlastním DMA zařízením. Samostatná aplikace na sdíleném kódu `../common/`, modul `zombBegin/zombLoop/zombEnd` připravený na launcher.

Stav: **první hratelná verze** (všechny mechaniky níže, ladění hodnot v `config.h`). Grafika je v `grafika-v3/` (balíček `krysy-zombici-amoled-v3.zip` bez zdrojových předloh; předávací pravidla v `grafika-v3/PRO_CLAUDA.md`), požadavky na další grafiku v `zadani-sprity.md`. Sprity se do kódu generují skriptem: `python3 gen_gfx.py grafika-v3 zomb_gfx.h`.

## Herní princip

Krysa na kolech jede zleva doprava po zvlněném terénu, pozadí ubíhá. Cílem je dojet do další **garáže**; cestou sbírá **body** (mince), řeší **početní příklady** za benzín navíc a projíždí **zombíky**, kteří ji zpomalují. Když dojde benzín, hra se vrátí do poslední garáže, kde hráč nasbírané body utratí za vylepšení a zkusí to znovu. Pět garáží = jeden **level**; po levelu se odemkne další krysa. Tři krysy: **chlupatá**, **dřevěná**, **ocelová**.

## Ovládání

| Vstup | Účinek |
|---|---|
| držení BOOT | plyn: zrychluje až na maximální rychlost krysy; puštění = pomalé zpomalování (odpor) |
| ťuknutí na displej | výskok (jen když jsou kola na zemi); dá se přeskočit zombík nebo vyjet na kládu, potrubí, kontejner, rampu |
| ťuknutí v panelu příkladu | volba odpovědi A / B / C (tři svislé dotykové zóny) |
| ťuknutí v garáži | `+` pod ikonou koupí další úroveň; ťuknutí na dveře garáže = vyjet |
| ťuknutí v úvodu | výběr krysy (odemčené), start |

## Krysa a její tři vlastnosti

Každá vlastnost má úroveň **1–10** a kupuje se v garáži za body.

| Vlastnost | Ikona v garáži | Co dělá | Vzorec (návrh, ladí se v `config.h`) |
|---|---|---|---|
| motor | `garage_engine` | maximální rychlost | `V_MAX = 50 + 6·(motor−1)` bodů/s (50 → 104); zrychlení `A = 25 + 3·(motor−1)` bodů/s² |
| nádrž | `garage_fuel_can` | dojezd | objem `TANK = 1500 + 250·(nádrž−1)` bodů dráhy při odporu úrovně 1 (1500 → 3750) |
| kola | `garage_wheel` | menší odpor → delší dojezd, rychlejší rozjezd na bahně | spotřeba `×(1 − 0,06·(kola−1))` (úroveň 10 = −54 %); na bahně zpomalení `−20 %·(1 − (kola−1)/9)` |

Benzín ubývá s ujetou dráhou (ne s časem), takže stání nestojí nic; do kopce se spotřeba zvyšuje o sklon (`×(1 + 1,5·sklon)`), z kopce klesá. Krysy se liší výchozí úrovní vlastností a vzhledem:

| Krysa | Odemčení | Výchozí úrovně (motor / nádrž / kola) | Rychlost | Zpomalení zombíky | Sprity |
|---|---|---|---|---|---|
| chlupatá | od začátku | 1 / 1 / 1 | ×1,0 | ×1,0 | `rat_furry_*` |
| dřevěná | po levelu 1 | 3 / 3 / 3 | ×1,15 | ×0,7 | `rat_wood_*` |
| ocelová | po levelu 2 | 5 / 5 / 5 | ×1,3 | ×0,45 | `rat_steel_*` |

Vylepšení se počítají zvlášť pro každou krysu (nová krysa začíná na svých výchozích hodnotách, body se ale přenášejí).

## Levely, garáže a body

- Level = 5 úseků (`GARAGES_PER_LEVEL`), každý končí garáží s vlastním jménem. Vzdálenost mezi garážemi `SEGMENT_LEN = 1800` bodů dráhy (+ 200 za každý další level). Krysa úrovně 1/1/1 (dojezd 1500) první garáž nedojede — musí nasbírat body, v startovní garáži koupit nádrž a kola a zkusit to znovu. To je zamýšlená smyčka.
- Jména garáží: level 1 „U Hrbáče", „Rezavá díra", „Stará pumpa", „U tří kol", „Plecháček"; level 2 „Kocourkov", „Za komínem", „Plechová bouda", „U křivé lampy", „Smeták"; level 3 „Konec světa", „Šrotiště", „Sto chlupů", „Díra v plotě", „Poslední stanice". Jméno se ukáže na cedulce při příjezdu i v garáži.
- **Body** = mince (`coin`), 1 bod každá; leží v řadách po 3–5 na terénu a nad překážkami. Za úsek jich je ~35–45. Správně vyřešený příklad dává 2 body navíc, zásah zombíka bourákem 3 body.
- **Cena vylepšení** z úrovně `n` na `n+1` je `10·n` bodů (1→2 = 10, 5→6 = 50, 9→10 = 90; celá cesta 1→10 = 450). Za první úsek se tedy dá koupit nádrž 2 a kola 2, po druhém pokusu obvykle další dvě úrovně.
- Když dojde benzín: krátká animace zastavení, hláška „DOSEL BENZIN", **dočasná garáž** (stejný nákup vylepšení, nadpis DOCASNA GARAZ) **s nasbíranými body** (mince z nedokončeného úseku se počítají) a pak návrat k záchytnému bodu = poslední dosažené hlavní garáži (na začátku levelu start). Úsek se generuje znovu (jiné rozestavení).
- Dojezd do garáže: benzín se doplní do plna, hráč nakupuje a vyjíždí do dalšího úseku.
- Po páté garáži levelu: obrazovka „LEVEL HOTOV", odemčení další krysy, výběr krysy.

## Předměty na trati

| Předmět | Sprite | Účinek | Četnost na úsek (level 1) |
|---|---|---|---|
| mince | `coin` | +1 bod | 35–45 (řady po 3–5) |
| znaménko plus / krát | `pickup_plus`, `pickup_multiply` | zastaví hru, sčítací / násobicí příklad | 1 s pravděpodobností `MATH_PCT_SEG` = 40 % na úsek |
| kanystr | `fuel_can` | +5 % nádrže | 2 |
| ocelový bourák | `wrecking_ball` | připevní se na ocas, krysa jím před čumákem mlátí zombíky 16 s od sebrání (`BALL_S`), pak zmizí | 1 |

Počet mincí a zombíků s levelem roste.

### Příklad (matematika)

Po sebrání znaménka hra zastaví a ukáže panel `math_panel` s příkladem `a + b` nebo `a × b`, `a, b ∈ 0..9`. Pod ním tři odpovědi **A B C** (`answer_a/b/c`) v náhodném pořadí: jedna správná, dvě špatné (správný výsledek ± 1..3, u násobení také sousední násobek; vždy různé a nezáporné). Panel je přes celý displej (okraj 2 body, zaoblené rohy), odpovědi velkým písmem v rámečcích. Ťuknutí do třetiny displeje = volba. Správně: **+10 % nádrže** (může přetéct nad plno až do 120 %), +2 body, zelený palec nahoru a stoupající tón; špatně: červený křížek a hluboké dvojí pípnutí. Výsledek se ukáže 0,6 s, pak hra pokračuje.

## Zombíci

Šest druhů ve třech siluetách (z balíčku je 9 variant, hra použije vždy dva z každé silueti podle levelu):

| Silueta | Sprity | Zpomalení při dotyku | Chůze | Na úsek (level 1 → 3) |
|---|---|---|---|---|
| hubený | `zombie_thin_stripes`, `_mechanic`, `_punk` | −10 bodů/s | 8 fps | 8 → 12 |
| hranatý | `zombie_block_worker`, `_denim`, `_sport` | −18 bodů/s | 7 fps | 3 → 6 |
| kulatý | `zombie_round_shirt`, `_janitor`, `_armor` | −28 bodů/s | 6 fps | 1 → 4 |

- Zombík vrávorá doleva (proti kryse) rychlostí 4–8 bodů/s, šestifázová chůze (`*_walk_0..5`, kotva 20,43 na plátně 40×44).
- **Dotyk s krysou**: rychlost krysy klesne o uvedené zpomalení (ne pod 8 bodů/s), zombík se **rozpadne**: hlava, trup, dvě ruce, dvě nohy (`parts/`, polohy dílů v aktuálním snímku z `atlas.json → part_offsets`). Hlava dostane největší vzestup, díly letí s gravitací, jednou se odrazí, pak leží a odjíždějí s terénem doleva; s hráčem už nereagují. Nejvíc 12 aktivních dílů najednou (starší zmizí).
- **Přeskočení** (výskok) zombíka = žádné zpomalení.
- **S bourákem**: švih `wrecking_swing_0..7` (15 fps) se spouští automaticky, když je zombík do 30 bodů před čumákem; zásah = rozpad bez zpomalení, +3 body, jiskry `impact_0..3`.

## Terén a překážky

- Výšková funkce `h(x)` = součet 2–3 sinusovek s náhodnými fázemi + občasný kopec; amplituda roste s levelem (±6 → ±12 bodů). Povrch se kreslí po sloupcích z pásu `mud_repeat` / `concrete_repeat` (úseky bahna a betonu se střídají po 150–300 bodech; bahno brzdí podle kol).
- Krysa má dvě kola (rozvor 28 bodů); každé kolo sleduje `h(x)`, podvozek se natočí podle spojnice kol (celočíselně, ±20°, kreslí se z předpočítaných snímků `*_drive_0..5` s natočením v kódu jen v 5 krocích). Skok: vertikální rychlost `JUMP_V = 150` bodů/s (výška ~37 bodů), gravitace 300; přistání zpět na `h(x)` nebo na horní hranu překážky. Skok jde i ze stání (po nárazu do kontejneru).
- Překážky s profilem, po kterých se dá jet nebo na ně vyskočit: kláda `log`, potrubí `pipe`, kontejner `container`, rampa `ramp` (odraz nahoru), U-rampa `halfpipe` (jen kulisa). Náraz do čelní stěny kontejneru bez výskoku = zastavení; při stoupání ve skoku stěna nevadí. Při nárazu do zombíka stříká krev (částice).
- Kulisy ve třech vrstvách paralaxy: daleké město `city_far_bg` (0,2×), tlumené `*_bg` domy / továrna / chatrče / U-rampa / plot (0,5×), blízko plně barevné ploty, popelnice, kontejnery, U-rampa, palety, lampy, cedule, kužely, lidé, psi, kočky (1×). Na obloze mraky (0,1×) a holubi `pigeon_fly_0..3` (9 fps).
- **Barvy pozadí** (požadavek po náhledu v1): **modré nebe** (svislý přechod `blue_light` → `sky_light` kreslený hrou) a **tmavší pás pod domy** mezi kulisami a terénem (`earth_dark` / `shadow`), aby kulisy stály na zemi a nesplývaly s terénem. Viz `zadani-sprity.md`.

## Obrazovky

1. **Úvod / výběr krysy**: tři krysy vedle sebe, zamčené šedě; ťuknutí = volba a start v první garáži.
2. **Jízda**: HUD nahoře — palivoměr `fuel_gauge_0..10` vlevo, číslo levelu, body s ikonou mince vpravo; při bouráku malá ikona a odpočet.
3. **Příklad**: zastavená scéna, panel s příkladem a A/B/C.
4. **Garáž**: cedule se jménem garáže, tři ikony 30×30 se středy x = 27, 75, 123, pod nimi úroveň 1–10, cena a `button_upgrade` (šedé, když nejsou body nebo úroveň 10), dole „TUKNI DOLE = JET". Ťuknutí na plus bez dostatku bodů otevře okénko „UZ NEMAS BODY", další ťuknutí rovnou vyjede.
5. **Došel benzín**: zastavení, hláška, návrat do garáže.
6. **Level hotov**: jméno levelu, odemčená krysa, pak výběr krysy.

## Zvuk

Sdílený `../common/amoled_audio.h` (`AUDIO_VOLUME` 60: hodnota registru kodeku je v krocích 0,5 dB, 38 jako u krysy skokana je −47 dB a skoro neslyšitelných), syntéza jako u krysy skokana: motor (obdélník s výškou podle rychlosti, tlumený), skok, dopad, mince, správná / špatná odpověď, zombík (mlasknutí + rozpad), bourák (švih + úder), kanystr, došel benzín (motor zhasne), nákup v garáži, fanfára levelu. Bez hudby.

## SD karta

Postup hry se **neukládá**, po zapnutí se jede vždy od začátku (chlupatá krysa 1/1/1, level 1). Na kartě (SDMMC 1 bit, `zomb_save.h`) je jen nastavení CRT filtru `/rat-zombies/settings.txt`.

## Grafika a paměť

- Balíček v1: 273 nativních PNG, paleta 48 barev (`grafika-v3/palette.json`), animace a kotvy v `atlas.json`, `animations.json`, `rigs.json`. Postavy jsou celé snímky (nemusí se skládat za běhu), rozpad používá `parts/`.
- Do kódu se převede generátorem jako u sady ASTRA (PNG → 8bitové indexy do palety hry; ne ASCII, na to je spritů moc): `const uint8_t` pole ve flash, ~240 KB. Interní RAM zůstává na canvas 18 KB, DMA pruhy a stav hry.
- Rozměry hlavních spritů: krysa 46×24 (kotva 23,23), zombík plátno 40×44, garážové ikony 30×30, panel příkladu 100×40, palivoměr 26×7, terénní pás 48×14, bourák švih 66×48.
- CRT filtr z krysy skokana **ano** (`zomb_crt.h` je jeho kopie s paletou z `zomb_gfx.h` a parametry za běhu); rychlý režim, ~31 fps. **Swipe dolů** kdekoli vylosuje novou náhodnou kombinaci filtru (`zomb_crt_presets.h`, jako u rakety): dva režimy skládání: **LINKY** (výchozí, 70 %): měkké vodorovné řádky s prosvitem sousedních řádků a lehkým vodorovným rozmazáním, tedy záře jako u rakety, bez RGB masky; **TECKY**: tvrdé scanlines, RGB maska (triády) nebo bez ní, putující pruh jasu, blikání. Vždy navíc gama palety a jiskry; bez vinětace. Režim LINKY je přes tabulky dvojic barev (`pairRow[p]`, `pairCol` 64×64, RGB maska započítaná podle fáze p), sousední indexy jdou z posuvného okna, takže je rychlejší než TECKY (~50 fps, tabulky se nepřepočítávají každý snímek). **Swipe nahoru** vrátí oblíbenou kombinaci (`CRT_FAVORITES` v `zomb_crt_presets.h`); obojí se uloží na kartu. Pokusy o „skutečné CRT s tečkami a rozmazáním" (maska 150 až 210 s prosvitem a rozmazáním) se uživateli nelíbily.

### Oblíbené CRT pro toto zařízení

| Jméno | Režim | Řádky horní/dolní | Prosvit | Rozmazání | Maska / zesílení | Poznámka |
|---|---|---|---|---|---|---|
| **crt-favorite-1** (výchozí) | LINKY | 235 / 120 | 120 | 90 | 0 | měkké řádky se září, bez teček; uživatel: „to je dobré, super" |

- Nebe kreslí hra jako čtyři pásy modré (`C_SKY_TOP..C_SKY_HAZE` v paletě navíc), pod domy je tmavý pás `C_BAND` od `BAND_Y` k terénu.

## Struktura souborů

| Soubor | Obsah |
|---|---|
| `esp32-rat-zombies.ino` | tenký wrapper `hwInit(); touchBegin(); zombBegin()` / `touchRead(); zombLoop()` |
| `zomb_app.h` | modul `zombBegin/zombLoop/zombEnd`, vstup (BOOT = plyn, ťuknutí, převod dotyku na logické body, zóny A/B/C, garáž), načtení a ukládání postupu |
| `config.h` | mřížka, vzorce vlastností, ceny, četnosti, fyzika, CRT, zvuk |
| `zomb_gfx.h` (generované) | paleta (48 barev balíčku + nebe, pás, zelená/červená), 273 bitmap s kotvami, sekvence animací, díly zombíků a jejich polohy ve snímcích chůze |
| `zomb_world.h` | stav hry, terén (výšková funkce, bahno/beton, profily překážek), generátor úseku, fyzika krysy, benzín, zombíci a rozpad, mince a předměty, příklady, garáž a levely |
| `zomb_render.h` | vrstvy (nebe, mraky, holubi, město, domy, pás, terén po sloupcích, rekvizity, lidé), překážky, předměty, zombíci a díly, krysa natočená podle terénu, bourák, HUD, obrazovky |
| `zomb_crt.h` | kopie CRT modulu krysy skokana |
| `zomb_audio.h` | motor podle rychlosti + efekty |
| `zomb_save.h` | SD karta: nastavení CRT `/rat-zombies/settings.txt` (postup se neukládá) |
| `gen_gfx.py`, `grafika-v3/` | generátor `zomb_gfx.h` a zdrojový balíček grafiky |
| `build_opt.h` | `-O2` |
