# Fase 0 — bewijs op devkits

Doel: op devkits aantonen dat de architectuur klopt, vóórdat er een eigen print wordt besteld. Fase 0 beantwoordt twee vragen:

1. Werkt één Matter-thermostaat tegelijk, lokaal en stabiel in Apple Home én Home Assistant? (R1, R3)
2. Kunnen we de eigen Nefit-ketel via EMS lezen en aansturen vanuit eigen firmware?

OpenTherm is geparkeerd tot nader order (ADR-017): eerst werkt TherIQ voor het eigen huis. WP0.6 en WP0.7 staan
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

Controleer actuele beschikbaarheid en of de interfaces op 3,3 V werken met de ESP32-C6.

| Item | Aantal | Voor |
| --- | --- | --- |
| ESP32-C6-DevKitC-1 met WROOM-1-N8 of -N16 (≥ 8 MB flash; niet DevKitM-1, die heeft 4 MB). Bij N16 de partitietabel op 8 MB houden, zoals de ketelunit straks heeft | 1 + 1 reserve | Ketelunit-prototype; de reserve voor als er een sneuvelt of de ketelunit vastzit in de 48-uurstest |
| Waveshare ESP32-C6-Zero, esptool meldt ESP32-C6FH4 rev v0.2 met 8 MB flash (al in huis) | 1 | Direct inzetbaar voor WP0.1–0.3 (zelfde chip en flash als de ketelunit); daarna kamerunit-prototype (WP0.9) |
| USB-C-kabel met data | 1–2 | Flashen en seriële log |
| BBQKees EMS Interface Board V3.1 (€ 34,95, bbqkees-electronics.nl, gezien 2026-11-18) | 1 | WP0.4 met een klassieke ESP32-WROOM uit de la en EMS-ESP; daarna WP0.5 aan de C6 (ADR-013) |
| SHT45-breakout | 1 | WP0.3 |
| Logic analyzer + multimeter | 1 | EMS-timing en break op de UART debuggen; spanningen meten |

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

### WP0.6 — OpenTherm-driver en ketelsimulator (geparkeerd, ADR-017)

- `firmware/ketelsim`: ESP32 met slave-shield die zich als OT-ketel gedraagt, met instelbaar gedrag (welke ID's ondersteund, storingen, traag antwoord, geen antwoord).
- OT-master-driver op de C6 via de RMT-peripheral, achter dezelfde ketel-API.
- **Klaar als**: alle ID's uit SPEC §4 worden uitgewisseld; een "unknown data-ID" wordt onthouden en niet meer gevraagd; met de MCU in reset is de lijnspanning in de rusttoestand (gemeten); een testscript draait de scenario's automatisch.

### WP0.7 — OpenTherm tegen de echte ketel (geparkeerd, ADR-017)

- OT-master → EMS-OT-converter → eigen ketel. Directe EMS-route staat dan uit.
- **Klaar als**: de minimale berichtenset werkt tegen de converter; verschillen met de simulator staan in de ketellijst.

### WP0.8 — Lus sluiten (minimaal)

- Matter-setpoint en SHT45 sturen via de ketel-API een eenvoudige aan/uit-regeling met hysterese. PI en stooklijn zijn fase 1.
- Eenvoudig weekschema op het apparaat (minimaal: nachtverlaging), handmatige wijziging geldt tot het volgende schakelmoment (SPEC §6, regel 4).
- **Klaar als**: na akkoord van Aernout 48 uur stoken op de eigen ketel zonder ingrijpen, met log; bedienen kan vanuit Apple Home én HA.

### WP0.9 — Kamerunit-prototype aan de wand (mijlpaal 31-12, ADR-008)

- Devkit + SHT45 + e-paper (~2,9") + draaiknop in een 3D-geprinte behuizing (PETG/ASA), bediening volgens SPEC §6.
- Open vraag, vóór start beslissen: hoe het prototype aan de wand voeding en verbinding krijgt. De bestaande thermostaatkabel is nu de EMS-bus naar de Nefit-thermostaat; de echte kamerbus (D2) is fase 1–2. Opties: tijdelijk wifi + USB-voeding, of de oude thermostaat weghalen en de kabel hergebruiken.
- **Klaar als**: hangt aan de wand, toont kamertemperatuur en setpoint, draaien wijzigt het setpoint en dat staat binnen 2 s in Apple Home en HA.

## Na fase 0

Gate gehaald: resultaten in `docs/tests/`, beslissingen als ADR in `docs/DECISIONS.md`, daarna start fase 1 (ketelunit rev A, eigen PCB).
