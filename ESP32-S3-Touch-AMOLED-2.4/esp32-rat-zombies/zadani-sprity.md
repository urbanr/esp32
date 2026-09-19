# Krysy a zombíci – zadání grafiky (doplnění k sadě v1)

Sada `krysy-zombici-amoled-v3` (v repozitáři `grafika-v3/`) je základ a **zůstává**: paleta 48 barev, rozměry, kotvy, animace chůze a rozpad zombíků, tři krysy s běžnou i ozbrojenou jízdou, kulisy, UI. Tento dokument shrnuje, co se má **upravit** a co **chybí**. Technické podmínky stejné jako u v1: nativní PNG 1 bod = 1 px, binární alfa, paleta `palette.json`, `atlas.json` s rozměry a kotvou, pojmenování podle tabulek.

## Úpravy podle náhledu v1

1. **Modré nebe.** V náhledu je obloha šedozelená (`sky`, `sky_light`). Nebe kreslí hra jako svislý přechod, ale kulisy s ním musí ladit: přidat do palety dvě modré oblohy (návrh `sky_blue` #5c9ad8, `sky_blue_light` #9cc8ec) a přebarvit mraky `cloud_0/1` a daleké město `city_far` / `city_far_bg` tak, aby na modré seděly (mraky bílé s modravým stínem, město tmavší modrošedá silueta).
2. **Tmavší spodek pod domy.** Pás mezi domy a terénem (kde domy „stojí") má být tmavý (`earth_dark` #403c2b nebo `shadow` #383c44), ne světlý. Domy, továrna, chatrče a garáže mají mít ve spodních 2–3 řádcích tmavý sokl / stín, aby na tmavém pásu seděly. Terénní pásy `mud_*` a `concrete_*` mají mít spodní polovinu tmavší (přechod do `earth_dark`), protože pod nimi je až ke spodnímu okraji tma.

## Co chybí (nové sprity)

| Soubor | Rozměr | Popis |
|---|---|---|
| `garage_sign` | 60×14 | prázdná cedule nad dveřmi garáže, jméno doplní hra fontem 3×5 |
| `garage_inside` | 150×123 | pozadí obrazovky garáže: vnitřek dílny (regály, nářadí, žárovka), uprostřed volné místo pro tři ikony 30×30 se středy x 27, 75, 123 na y 55 a čísla pod nimi |
| `garage_door_open`, `garage_door_closed` | 42×28 | dveře garáže pro animaci vjezdu / výjezdu (k `garage_red/teal/yellow`) |
| `rat_select_frame` | 50×32 | rámeček výběru krysy; `rat_locked` 42×18 šedá silueta zamčené krysy (stačí jedna, hra ji přebarví) |
| `fuel_empty_0..3` | 18×14 | obláček z výfuku a prskání motoru, když dojde benzín, 4 fáze |
| `coin_spin_0..3` | 9×9 | otáčení mince (pohled zepředu → hrana → zezadu → hrana); v1 je jen statická |
| `ball_pickup_0..1` | 11×14 | ocelový bourák na trati, 2 fáze lesku (v1 je jen `wrecking_ball` bez animace) |
| `hud_ball` | 9×9 | malá ikona bouráku do HUD s odpočtem |
| `hud_coin` | 7×7 | malá mince k číslu bodů (v1 `coin` 9×9 je velká) |
| `check_ok`, `check_bad` | 11×11 | zelená fajfka / červený křížek po odpovědi |
| `mud_splash_0..3` | 14×8 | blátivý cákanec od kol na bahně, 4 fáze (na betonu se používá `dust_*`) |
| `zombie_*_hit` | 40×44 | volitelně: jeden snímek „zásah" pro každý ze 6 použitých zombíků (oči vyvalené, ruce nahoru) zobrazený 2 snímky před rozpadem |
| `level_banner` | 120×30 | rámeček „LEVEL HOTOV" / název levelu |
| `font_5x7.png` | 5×7 na znak | velké číslice 0–9 a znaky `+ × = A B C` pro panel příkladu (font 3×5 z v1 je na příklad malý; při 3× je 9×15 px) |

## Co má být ve výsledné sadě jinak než ve v1 (jen pokud se překresluje)

- Psi, kočky a lidé v pozadí: aspoň 2 snímky (pes vrtí ocasem, kočka švihá ocasem, člověk pokývá hlavou), 2 fps.
- Holub: ponechat `pigeon_fly_0..3`.
- Rampa `ramp`: sjezd vlevo, nájezd vpravo, aby se dala přejet zleva doprava.

## Výstup

ZIP jako v1: `png/`, `parts/`, `tiles/`, `palette.json`, `atlas.json`, `animations.json`, `preview.png` s ukázkou scény na **modré obloze s tmavým pásem pod domy**.
