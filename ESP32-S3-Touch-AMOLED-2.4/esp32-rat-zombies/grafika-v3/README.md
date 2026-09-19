# Krysy a zombíci — AMOLED sprity v3

Opravená sada v3: čtyři skutečně nově vytvořené kontrastní předlohy D, ze kterých teprve vznikl export pro malý AMOLED. Vychází ze schváleného výtvarného plátna a synových tří tvarů zombíků. Obsahuje 273 nativních PNG včetně jednotlivých animačních snímků a oddělených částí; nejde o 273 různých postav.

## Oprava předloh

V3 nahrazuje nesjednocené podklady předchozích sad novými D kresbami postav, prostředí, předmětů a terénu. Přílohy `source-reference/D-characters.png`, `D-environment.png`, `D-props.png` a `D-terrain.png` jsou nové originály z této opravy. `source-map.json` uvádí u všech 273 PNG původ a odvození. Základní chlupatá krysa a dělník jsou přesně zachované schválené malé D, nikoli starší měkké předlohy.

Dřevěná a ocelová krysa vycházejí z nové D kresby, mají vysoké zaoblené tělo a stejný výstupní rozměr 42 × 21 px jako první krysa. Animace a díly byly znovu sestaveny z této sady. Nativní jednoduchá UI a efekty mají vlastní záznam původu; nejsou vydávány za export z obrazové předlohy.

## Rychlé otevření

- `nahled.html` — samostatný lokální pohyblivý náhled. Stačí otevřít v prohlížeči, nepotřebuje internet ani server. Přepínání krysy/zombíka, pauza, bourák a ukázka rozpadu.
- `previews/animace.gif` — krátká animovaná ukázka.
- `previews/screen-448x368.png` — skutečná velikost displeje.
- `previews/garage-icons.png` — motor, kolo a kanystr s ukázkovou úrovní a tlačítkem plus.
- `previews/tri-krysy-3x.png` — všechny tři opravené krysy při skutečném měřítku 3×.
- `previews/D-predlohy.png` — společný přehled čtyř nových D předloh.
- `previews/catalogue.png` — všechny základní sprity.
- `previews/walking-frames.png` — přehled všech snímků chůze.
- `PRO_CLAUDA.md` — pravidla načítání, kotvení, animací a rozpadu.

## Schválená velikost

Displej 448 × 368 px na šířku, zvětšení 3×. Zachována logická mřížka původního zadání 150 × 123 bodů. Její zvětšený obraz má 450 × 369 px: oříznout 2 px vpravo a 1 px dole; nepřepočítávat celý obraz necelým měřítkem. Důležitý obsah držet v oblasti x 0–148, y 0–121.

Každé nativní PNG má 1 herní bod = 1 pixel. Při zobrazení používat pouze nearest-neighbor. PNG mají alfa kanál pouze 0 nebo 255 a společnou paletu 48 barev. Průhledné pixely nemají vykreslenou šachovnici.

Velké garážové ikony mají přesně **30 × 30 nativních pixelů**, tedy **90 × 90 px na displeji**: `garage_engine`, `garage_wheel`, `garage_fuel_can`. Motiv je vycentrován s drobným průhledným okrajem. Malé `engine`, `wheel`, `fuel_can` slouží pro jiné části hry.

## Obsah

- 3 krysy: chlupatá, dřevěná, ocelová; běžná i ozbrojená varianta jízdy.
- 9 zombíků: 3 hubení, 3 hranatí, 3 kulatí. Každý má 6 snímků stylizované vrávoravé chůze a 6 samostatných částí pro rozpad.
- Domy, továrna, dvě chatrče, 3 garáže, vzdálené město, U-rampa, nájezd, kontejner, popelnice, potrubí, kláda, palety, lampa, plot, bedny, sud, odpadky, tráva, kameny a značka.
- 2 psi, 2 kočky, 3 lidé, holub se třemi pózami křídel a čtyřsnímkovým cyklem, mraky.
- Bonusy, velké garážové ikony, odpovědi A/B/C, tlačítko plus, prázdný panel příkladu, 11 stavů palivoměru, základní font 3 × 5.
- Bahnitý a betonový pás včetně opakovatelné varianty. Tvar svahu určuje hra, nikoli pevný obrázek.
- Švih bouráku, prach a zásahové jiskry.
- 15 tlumených variant kulis s příponou `_bg`, aby prostředí nepřehlušilo hráče.

## Formáty

`png/` obsahuje postavy, objekty, UI a animační snímky. `parts/` obsahuje části zombíků a vozidel. `tiles/` obsahuje terén. `atlas.json` je hlavní seznam rozměrů, ukotvení a cest. `animations.json` popisuje pořadí snímků a doporučené rychlosti. `rigs.json` popisuje části a jejich sestavení.

Volitelně lze použít `spritesheet.png` + `spritesheet.json` místo jednotlivých souborů. Tento přehledový atlas má 256 × 1365 px; konkrétní zařízení může vyžadovat rozdělení nebo načítání jen potřebných položek.

`indexed/` je úsporný export: 1 byte na pixel, 0 = průhlednost, další hodnoty odpovídají paletě. Přiložena RGB565 little-endian paleta. Všechna nativní pixelová data dohromady mají 238 540 bytů, paleta 98 bytů; to nezahrnuje souborový systém, metadata, framebuffer a paměť programu. Není nutné držet v RAM všechny postavy a snímky současně.

## Co je ověřeno a co zbývá

Ověřeny rozměry všech PNG, binární alfa, společná paleta, garážové ikony 30 × 30, stabilní rozměry a kotvy animačních cyklů, přesné složení devíti zombíků z dílů a shoda krajních sloupců opakovatelných dlaždic. Náhled byl spuštěn v prohlížeči a kontrolován vizuálně. Strojové výsledky jsou v `validation.json`.

Chůze je první šestifázová stylizovaná animace sestavená z posunů jednotlivých dílů. Není to podrobná ručně překreslená animace každého kloubu. Psi, kočky a lidé jsou v této sadě statičtí. Výkon na fyzickém zařízení, kolize, jízdní fyzika a herní vyvážení zatím ověřeny nejsou. Ukázkový rozpad v HTML se po chvíli resetuje; chování skutečné hry upraví implementace.

Schválené detailní návrhy pro další rozvoj jsou v `source-reference/`. Pro větší zařízení se má vycházet z těchto předloh a znovu připravit vhodné sprity, nikoli vyhlazovat malé PNG.

Grafické předlohy vznikly vestavěným nástrojem imagegen. Následoval technický export do malého rastru, sjednocení palety, rozdělení na části, sestavení animací a kontrola. Znění výtvarných zadání a exportní specifikace jsou v `GENEROVANI.md`.
