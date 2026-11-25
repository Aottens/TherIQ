# Fase 0 — bewijs op devkits

Doel: op devkits aantonen dat de architectuur klopt, vóórdat er een eigen print wordt besteld. Fase 0 beantwoordt twee vragen:

1. Werkt één Matter-thermostaat tegelijk, lokaal en stabiel in Apple Home én Home Assistant? (R1, R3)
2. Kunnen we de eigen Nefit-ketel via EMS lezen en aansturen vanuit eigen firmware?

OpenTherm is geparkeerd tot nader order (ADR-019): eerst werkt TherIQ voor het eigen huis. WP0.6 en WP0.7 staan
hieronder nog als referentie, maar worden niet gepland.

Gate fase 0: R1 en R3 gemeten, EMS lezen en schrijven op de eigen ketel.

## Uitgangssituatie

- Eigen ketel: **Nefit TopLine Compact HRC 25/CW4**, spreekt **EMS**, geen native OpenTherm (D0).
- Huidige Nefit-thermostaat hangt op dezelfde EMS-bus. Die blijft voorlopig zitten als terugvaloptie.
- Home Assistant is beschikbaar, maar nog niet volledig ingericht. Apple Home met woninghub is in gebruik.

## Veiligheidsregels voor fase 0

- **Eerst alleen lezen.** Niets naar de echte ketel schrijven zonder expliciet akkoord van Aernout per test.
- **Eén regelaar tegelijk.** Nooit twee partijen (bestaande thermostaat, EMS-ESP, eigen firmware, EMS-OT-converter) tegelijk setpoints laten schrijven.
- **Terugweg vooraf klaar.** Bij elke schrijftest is beschreven hoe de oude situatie in één handeling terugkomt.
- De ketel houdt zijn eigen beveiliging. Wij sturen alleen setpoints.

## Inkooplijst (indicatief)

Controleer actuele beschikbaarheid en of de interfaces op 3,3 V werken met de ESP32-C6. Liefst Europese winkels: bestellingen uit
China en de VS gaven invoerkosten en vastgehouden pakketten (2026-11-23). HobbyElectronica (NL) levert de ketelunit-devkit, de
encoder en de voeding voor de kamerunit; BerryBase (DE) de SHT45 en het e-paper; TinyTronics (NL) de logic analyzer.

HobbyElectronica, gezien 2026-11-25: geen SHT45 (wel SHT30, te onnauwkeurig voor TQ-08), geen logic analyzer, e-paper alleen 4,2"
(€ 54,95). De C6 SuperMini daar heeft maar 4 MB flash en valt af. De DevKit C staat er als "ESP32 C6" zonder streepje: zoeken op
"esp32-c6" vindt hem niet.

| Item | Aantal | Voor |
| --- | --- | --- |
| ESP32-C6-DevKitC-1-kloon met ESP32-C6-WROOM-1-N16, 16 MB flash, 2× USB-C, RGB-led (€ 9,95, HobbyElectronica HE1251 "ESP32 C6 DevKit C 16 MB", 17 op voorraad, gezien 2026-11-25). Alternatief: Waveshare ESP32-C6-DEV-KIT-N8 (€ 8,90, BerryBase WS-25723). Niet de DevKitM-1 of bordjes met 4 MB | 1 | Ketelunit-prototype. Minstens 8 MB is nodig: de firmware is nu 1,83 MB en moet twee keer passen voor OTA |
| Klassieke ESP32-WROOM uit de la (al in huis) | 1 | Eerst EMS-ESP voor WP0.4; komt vrij als het BBQKees-bord naar de C6 gaat (WP0.5) |
| Waveshare ESP32-C6-Zero, esptool meldt ESP32-C6FH4 rev v0.2 met 8 MB flash (al in huis) | 1 | Direct inzetbaar voor WP0.1–0.3 (zelfde chip en flash als de ketelunit); daarna kamerunit-prototype (WP0.9) |
| USB-C-kabel met data (al in huis) | 1 | Flashen en seriële log |
| BBQKees EMS Interface Board V3.1 (al in huis, 2026-11-25) | 1 | WP0.4 met een klassieke ESP32-WROOM uit de la en EMS-ESP; daarna WP0.5 aan de C6 (ADR-013) |
| SHT45-breakout (Adafruit 5665, I²C, STEMMA QT; € 14,90, BerryBase ADA5665, gezien 2026-11-23) | 1 | WP0.3; TQ-08 meet of hij binnen 0,3 °C blijft. Gaat daarna mee met de C6-Zero naar de kamerunit |
| Logic analyzer 8 kanalen, 24 MHz (kloon, werkt met PulseView; € 9,75, TinyTronics LOGIC8) + multimeter (al in huis) | 1 | EMS-timing en break op de UART debuggen; spanningen meten; nodig vanaf TQ-14 |
| E-paper 2,9", 296×128, zwart-wit, SPI (Waveshare Pico-ePaper-2.9; € 17,90, BerryBase WS-19408, 3 op voorraad, gezien 2026-11-25). Zwart-wit omdat driekleurenpanelen geen gedeeltelijke verversing kennen; de losse module EINKM-29 was niet leverbaar. De Pico-header wordt met jumperdraden op de C6-Zero aangesloten | 1 | Kamerunit, TQ-27 (SPEC §6: nieuw setpoint direct zichtbaar) |
| Rotary encoder module, 20 stappen, KY-040-type (€ 2,45, HobbyElectronica HE0475-004, gezien 2026-11-25). Passieve module met pull-ups, werkt ook op 3,3 V; bij levering controleren of de drukknop (SW-pin) erop zit | 1 | Kamerunit, TQ-27 |
| Voeding 12 V 1 A (€ 8,95, HobbyElectronica, gezien 2026-11-25) | 1 | Ketelkant van de thermostaatkabel (ADR-018); pas nodig vanaf WP0.8 |
| Step-down LM2596S, regelbaar (€ 1,75, HobbyElectronica, gezien 2026-11-25); de kleinere MP1584EN was niet leverbaar | 1 | Wandkant: 12 V naar 5 V voor de C6-Zero (ADR-018); vóór aansluiten op 5,0 V afregelen met de multimeter |
| USB-netadapter (al in huis) | 1 | ESP bij de ketel voeden; het BBQKees-bord is niet galvanisch gescheiden |

## Werkpakketten

Elk werkpakket is klaar als het done-criterium aantoonbaar is gehaald. De werkpakketten zijn opgesplitst in taken (TQ-nn) in `docs/BACKLOG.md`. Checklists en testverslagen gaan in `docs/tests/`.

### WP0.1 — Repo en toolchain

- Repo-structuur volgens SPEC §9; licentiebestanden (GPL-3.0-or-later, CERN-OHL-S-2.0, CC BY-SA 4.0); SECURITY.md.
- ESP-IDF en esp-matter op vastgepinde, actuele stabiele versies, in een devcontainer of Docker-image zodat de build overal gelijk is.
- CI (GitHub Actions) bouwt `firmware/ketelunit` voor `esp32c6`.
- **Klaar als**: CI bouwt een lege Matter-app groen, en dezelfde build draait geflasht op de devkit.

### WP0.2 — Matter-thermostaat met nepsensor

- Device type Thermostat, alleen verwarmen: modi Heat en Off, setpoint, kamertemperatuur.
- Temperatuur uit een nepbron (vaste waarde of langzame sinus), gesynchroniseerd vóórdat de Matter-stack start (SPEC §3, valkuilen).
- Fabrieksreset via BOOT-knop 10 s.
- **Klaar als**: koppelen in Apple Home lukt; daarna via multi-admin ook in HA; een setpointwijziging in de ene app staat binnen 2 s in de andere (R3); met de WAN-kabel uit de router blijft alles werken (R1).

### WP0.3 — Echte kamertemperatuur

- SHT45 via I²C, gemiddeld en gefilterd, naar het Matter-attribuut voor kamertemperatuur en de luchtvochtigheid.
- **Klaar als**: waarde in Apple Home en HA wijkt minder dan 0,3 °C af van een referentiethermometer naast de sensor (R9 komt pas in fase 2 met eigen print).

### WP0.4 — De eigen ketel leren kennen (alleen lezen)

- BBQKees EMS Interface Board V3.1 met een klassieke ESP32-WROOM (4 MB) en de standaardfirmware EMS-ESP (`ESP32-4MB`-build) aan de ketel, parallel aan de bestaande thermostaat (ADR-013).
- Vastleggen: welke waarden de ketel geeft, welke schrijfbaar zijn, hoe de bestaande thermostaat op de bus praat, pollritme en antwoordtijden.
- **Klaar als**: `docs/ketels/nefit-topline-compact-hrc25-cw4.md` bestaat, met een lijst lees-/schrijfwaarden en een ruwe buslog van minimaal 24 uur in `docs/ketels/logs/`.

### WP0.5 — Ketel-API en EMS-driver in eigen firmware

- Ontwerp eerst de abstracte ketel-API (C++-interface): aanvoersetpoint schrijven, CV aan/uit, status, modulatie, aanvoer/retour, druk, buitentemperatuur, storing. OpenTherm en EMS worden implementaties hiervan.
- EMS-driver op de C6, met protocolkennis uit EMS-ESP (GPL-3.0, bronvermelding in de code en in NOTICE).
- Hetzelfde BBQKees EMS Interface Board V3.1 uit WP0.4 gaat aan de C6 (ADR-013).
- **Klaar als**: eigen firmware leest aanvoer, retour en modulatie van de eigen ketel; daarna, na akkoord van Aernout, schrijft hij een aanvoersetpoint en volgt de ketel dat aantoonbaar (log).

### WP0.6 — OpenTherm-driver en ketelsimulator (geparkeerd, ADR-019)

- `firmware/ketelsim`: ESP32 met slave-shield die zich als OT-ketel gedraagt, met instelbaar gedrag (welke ID's ondersteund, storingen, traag antwoord, geen antwoord).
- OT-master-driver op de C6 via de RMT-peripheral, achter dezelfde ketel-API.
- **Klaar als**: alle ID's uit SPEC §4 worden uitgewisseld; een "unknown data-ID" wordt onthouden en niet meer gevraagd; met de MCU in reset is de lijnspanning in de rusttoestand (gemeten); een testscript draait de scenario's automatisch.

### WP0.7 — OpenTherm tegen de echte ketel (geparkeerd, ADR-019)

- OT-master → EMS-OT-converter → eigen ketel. Directe EMS-route staat dan uit.
- **Klaar als**: de minimale berichtenset werkt tegen de converter; verschillen met de simulator staan in de ketellijst.

### WP0.8 — Lus sluiten (minimaal)

- Matter-setpoint en SHT45 sturen via de ketel-API een eenvoudige aan/uit-regeling met hysterese. PI en stooklijn zijn fase 1.
- Eenvoudig weekschema op het apparaat (minimaal: nachtverlaging), handmatige wijziging geldt tot het volgende schakelmoment (SPEC §6, regel 4).
- **Klaar als**: na akkoord van Aernout 48 uur stoken op de eigen ketel zonder ingrijpen, met log; bedienen kan vanuit Apple Home én HA.

### WP0.9 — Kamerunit-prototype aan de wand (mijlpaal 31-12, ADR-008)

- Devkit + SHT45 + e-paper (~2,9") + draaiknop in een 3D-geprinte behuizing (PETG/ASA), bediening volgens SPEC §6.
- Voeding en verbinding (ADR-018): de bestaande thermostaatkabel (2 aders) komt vrij als de oude Nefit-thermostaat in WP0.8 van de
  EMS-bus gaat. Daarna voedt de ketelunit het prototype over die 2 aders (bv. 12 V, buck-converter aan de wand); data via wifi.
  Tot dan op het bureau op USB. Aan de ketelkant gaan de aders eerst van de EMS-klemmen af. De echte kamerbus (D2) is fase 1–2.
- **Klaar als**: hangt aan de wand, toont kamertemperatuur en setpoint, draaien wijzigt het setpoint en dat staat binnen 2 s in Apple Home en HA.

## Na fase 0

Gate gehaald: resultaten in `docs/tests/`, beslissingen als ADR in `docs/DECISIONS.md`, daarna start fase 1 (ketelunit rev A, eigen PCB).
