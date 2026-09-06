# Integrace grafiky do hry Krysy a zombíci — v3

Použít kompletní v3 včetně palety, atlasu, částí a animací. Nemíchat soubory ani rozměry z v1/v2. Všechny základní krysy mají 42 × 21 px, animační plátno zůstává 46 × 24. Původ všech souborů je v source-map.json.

Tento soubor je předávací dokument ke grafice; herní pravidla mají vycházet ze zadání uživatele. Sada nepředepisuje ceny vylepšení, hustotu bonusů ani obtížnost.

## Zobrazení a paleta

Pracovat v 150 × 123 bodech, zvětšit celočíselně 3× a oříznout vpravo 2 fyzické px a dole 1 fyzický px na 448 × 368. Neprovádět interpolaci ani CRT filtr před ověřením čitelnosti. Pokud zařízení vyžaduje jinou orientaci framebufferu, změnit transformaci při odeslání na displej, ne jednotlivé PNG.

PNG, části a dlaždice mají nativní velikost. `atlas.json` obsahuje skutečné rozměry; nespoléhat na univerzální velikost postavy. Průhledné pixely přeskočit. Při převodu RGB565 zachovat samostatnou průhlednost. `indexed/format.json` popisuje již připravený jednobajtový export.

## Kotvení a animace

Pro sprite ukotvený ve světovém bodu `(x, y)` kreslit levý horní roh na `(x - anchor.x, y - anchor.y)`. Kotva zemních objektů je zpravidla ve spodním pixelu uprostřed. U dílů, UI a efektů se řídit explicitní hodnotou v atlasu.

Živí zombíci používají `zombie_*_walk_0` až `_5` na plátně 40 × 44 s kotvou (20, 43). Hubení mají doporučeno 8 fps, hranatí 7 fps a kulatí 6 fps. První a čtvrtá fáze záměrně sdílejí neutrální pózu. Rychlost přehrávání lze navázat na rychlost chůze.

Krysy používají `rat_*_drive_0` až `_5`, 46 × 24, kotva (23, 23). Rychlost otáčení kol má záviset na ujeté vzdálenosti. Při zastavení zmrazit aktuální snímek. Podvozek natočit podle spojnice míst kontaktu předního a zadního kola s terénem; náhled obsahuje pouze ilustrační naklánění. Všechny finální souřadnice rasterizovat celočíselně.

Holub: `pigeon_fly_0…3`, jednotné plátno 22 × 20, kotva (10, 10), 9 fps, cyklus nahoru–střed–dolů–střed. Posun po obloze je samostatná herní transformace.

## Rozpad zombíka

Každý zombík má `head`, `torso`, `arm_front`, `arm_back`, `leg_front`, `leg_back` v `parts/`. `rigs.json` uvádí názvy, klidové pozice a pořadí kreslení. Klidové sestavení dílů přesně obnoví základní PNG.

Každý snímek chůze má v `atlas.json` také `part_offsets`, tedy pozice dílů právě v této póze. Při zásahu použít tyto pozice, aby se postava při rozpadu nepřeskočila do klidové pózy.

1. Při prvním zásahu atomicky změnit stav živého zombíka na zasažený a ihned vyřadit jeho kolizi s hráčem. Zpomalení aplikovat pouze jednou.
2. Skončit vykreslování celého živého spritu; vytvořit šest částí na pozicích právě zobrazeného snímku.
3. Levý horní roh dílu ve světě = pozice kotvy zombíka − kotva animačního snímku + příslušný `part_offsets`.
4. Od tohoto okamžiku části vykreslovat nezávisle. Pro rotaci použít střed dílu jako pivot (nebo upravit souřadnice podle vlastního fyzikálního modelu).
5. Hlavě dát výraznější počáteční vzestup; ostatním částem menší rozptyl. Gravitační pohyb, odraz a úhlovou rychlost spočítá hra. Oddělené části nesmějí dál brzdit hráče.
6. Po dopadu nechat části ležet ve světě a posouvat je s kamerou doleva. Odstranit je po opuštění obrazovky; počet aktivních částí omezit podle zařízení.

Polohy ramen a nohou jsou stylizované, nikoli anatomický skeleton. Pro tuto malou velikost jsou dodána kompletní PNG chůze, aby nebylo nutné živé postavy skládat za běhu.

## Ocelový bourák

Při vybavení používat `rat_*_armed_drive_*`, kde je odstraněna vnější část běžného ocasu. Přes něj vykreslit `wrecking_swing_0…7`: 66 × 48, kotva (5, 41). Pro první přiblížení umístit kořen na bod `(rat.x - 13, rat.y - 8)` v souřadnicích nenatočené krysy. Při jízdě po svahu aplikovat na krysu i bourák stejnou transformaci.

`ball_center` v atlasu uvádí střed závaží v každém snímku. Světová poloha zásahu = světová kotva bouráku − kotva snímku + `ball_center`, následně transformovaná náklonem krysy. Pro kolizi použít okolí závaží; ne celý obdélník 66 × 48. Vhodný počáteční poloměr je 5 herních bodů, doladit hraním. Švih má doporučeno 15 fps; návrat může přehrát `return_frames`.

Alternativně použít samostatný `wrecking_ball` a ocas vykreslovat křivkou ve hře. Předpočítané snímky jsou k dispozici pro jednodušší vykreslování.

## Garáž

`garage_engine.png`, `garage_wheel.png`, `garage_fuel_can.png` mají přesně 30 × 30 px a kotvu (15, 15). Na displeji při 3× zabírají 90 × 90 px. V rámci logické šířky 150 se vejdou například se středy x = 27, 75, 123. Pod ikonou zobrazit úroveň 1–10 a `button_upgrade`. Náhled ukazuje pouze grafické rozvržení, ne ceny ani ovládání nákupu.

Malé `engine`, `wheel`, `fuel_can` slouží jako malé herní předměty; pro garáž je nezvětšovat místo velkých verzí. Matematický panel má prázdné pozadí, text příkladu a odpovědi doplní hra. `font-3x5.json` poskytuje číslice a operátory; není to kompletní český textový font.

## Prostředí a terén

Varianty `_bg` mají ztlumené barvy pro kulisy. Garáže mohou zůstat kontrastní jako cíle. Vzdálená silueta města, budovy a blízké objekty mají používat rozdílné rychlosti paralaxy.

`mud_repeat` a `concrete_repeat` mají shodný první a poslední sloupec; slouží jako povrchový pás. Zvlnění řešit výškovou funkcí a vykreslováním sloupců nebo segmentů. Grafická silueta rampy, potrubí či kontejneru není automaticky kolizní geometrie. Pro jízdu a skoky vytvořit jednoduché profily odpovídající horní ploše objektu.

## Stav ověření

Rozměry, paleta, alfa, kotvy cyklů a opakovací hrany jsou ověřeny v `validation.json`. Soubor `nahled.html` obsahuje referenční vizuální přehrávání, ale není výrobním herním enginem. Přístup ke zdrojům hry ani test na fyzickém AMOLED zařízení v tomto kroku nebyl proveden. Před finalizací hry ověřit čitelnost, rychlost dekódování, RAM a plynulost na skutečném zařízení.
