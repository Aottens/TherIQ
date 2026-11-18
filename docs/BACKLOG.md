# TherIQ — Backlog

Elke taak is één zin met een controleerbaar done-criterium. Grootte: S ≤ 2 u, M ≤ halve dag, L ≤ dag, XL = opsplitsen.
Status (eerste woord van de cel): `todo` · `doing` · `done` · `gate3` · `blocked`, details erachter.

- **Gemerged = klaar**: de PR zet zijn eigen rij op `done (datum, #PR)`. Geen status `review`.
- `gate3: <checklist>` = gemerged, alleen Aernouts run op hardware of de echte ketel ontbreekt nog; het pad bestaat.
- Een open punt uit een run wordt een eigen rij of IDEA in dezelfde PR, nooit een opmerking in een `done`-cel.
- Taken op de echte ketel die schrijven, zijn altijd gate 3 en vragen per test akkoord plus een beschreven terugweg.

**Builds** (een build = één sessie, één branch, één PR; zie CLAUDE.md):
Build A: TQ-04 + TQ-05 (gebouwd, wacht op gate 3).

## Fase 0 — bewijs op devkits

| Id | Taak (één zin, controleerbaar) | WP / eis | Grootte | Status |
| --- | --- | --- | --- | --- |
| TQ-01 | CI bouwt een lege Matter-app voor esp32c6 groen met de vastgepinde toolchain. | WP0.1 | S | done (2026-11-18) |
| TQ-02 | De CI-build draait geflasht op een C6-devkit; de log toont `TherIQ boiler unit started` en de koppelcodes. | WP0.1 | S | gate3: `docs/tests/devkit-flash.md` |
| TQ-03 | Host-side unit tests (pure logica, zonder ESP-IDF) draaien in CI als eigen job; één voorbeeldtest faalt aantoonbaar als de logica breekt. | conventie | S | todo |
| TQ-04 | Matter-endpoint Thermostat (Heat/Off, setpoint, kamertemperatuur) met nepsensor (langzame sinus), waarde gezet vóór `esp_matter::start`; build groen. | WP0.2 | M | gate3: `docs/tests/thermostat-flash.md` |
| TQ-05 | BOOT-knop 10 s ingedrukt wist fabrics en wifi; de RGB-led laat het aftellen zien. | WP0.2 | S | gate3: `docs/tests/thermostat-flash.md` |
| TQ-06 | Koppelen in Apple Home, daarna multi-admin in HA; setpoint wijzigen staat binnen 2 s in de andere app (R3); met WAN-kabel uit blijft alles werken (R1). | WP0.2, R1, R3 | S | todo |
| TQ-07 | SHT45-driver (I²C, CRC-controle, middeling) levert temperatuur en RV aan het Matter-endpoint; CRC en filter hebben host tests. | WP0.3 | M | todo |
| TQ-08 | Temperatuur in Apple Home en HA wijkt < 0,3 °C af van een referentiethermometer naast de sensor. | WP0.3 | S | todo |
| TQ-09 | BBQKees-interfacebord met EMS-ESP op een klassieke ESP32 hangt alleen lezend aan de ketel; ruwe buslog van ≥ 24 u staat in `docs/ketels/logs/`. | WP0.4 | M | todo |
| TQ-10 | `docs/ketels/nefit-topline-compact-hrc25-cw4.md` beschrijft lees-/schrijfwaarden, pollritme en antwoordtijden uit de log. | WP0.4 | S | todo |
| TQ-11 | Ketel-API (`boiler_api`) als C++-interface met ADR: setpoint, CV aan/uit, status, modulatie, aanvoer/retour, druk, buiten, storing. | WP0.5 | S | todo |
| TQ-12 | EMS-frame encode/decode en CRC met host tests (protocolkennis uit EMS-ESP, bronvermelding en NOTICE). | WP0.5 | M | todo |
| TQ-13 | EMS-interface voor de C6 gekozen en besteld. | WP0.5 | S | done (2026-11-18, ADR-013) |
| TQ-14 | EMS-driver (UART 9600, break-detectie) leest aanvoer, retour en modulatie van de eigen ketel. | WP0.5 | L | todo |
| TQ-15 | Na akkoord schrijft de firmware een aanvoersetpoint en volgt de ketel dat aantoonbaar (log); terugweg beschreven. | WP0.5 | M | todo |
| TQ-16 | OpenTherm-frame encode/decode en pariteit met host tests. | WP0.6 | S | todo |
| TQ-17 | `firmware/ketelsim` gedraagt zich als OT-ketel met instelbaar gedrag (ID's, storing, traag, geen antwoord). | WP0.6 | L | todo |
| TQ-18 | OT-master via RMT achter de ketel-API wisselt alle ID's uit SPEC §4 uit met de simulator. | WP0.6 | L | todo |
| TQ-19 | "Unknown data-ID" wordt onthouden; een testscript draait alle simulatorscenario's automatisch. | WP0.6 | M | todo |
| TQ-20 | Met de MCU in reset staat de OT-lijn gemeten in de rusttoestand. | WP0.6 | S | todo |
| TQ-21 | (optioneel) Minimale OT-berichtenset werkt via de EMS-OT-converter; verschillen staan in de ketellijst. | WP0.7 | L | todo |
| TQ-22 | Aan/uit-regelaar met hysterese als pure logica met host tests. | WP0.8 | S | todo |
| TQ-23 | Weekschema op het apparaat (minimaal nachtverlaging); handmatige wijziging geldt tot het volgende schakelmoment; host tests. | WP0.8 | M | todo |
| TQ-24 | Lus gesloten: Matter-setpoint en SHT45 sturen via de ketel-API de ketel aan/uit. | WP0.8 | M | todo |
| TQ-25 | Na akkoord 48 u stoken op de eigen ketel zonder ingrijpen, met log; bediening vanuit Apple Home én HA. | WP0.8 | S | todo |
| TQ-26 | Voeding en verbinding van het kamerunit-prototype beslist (ADR). | WP0.9 | S | todo |
| TQ-27 | E-paper en draaiknop op de Waveshare C6-Zero tonen kamertemperatuur en setpoint; draaien wijzigt het setpoint. | WP0.9 | M | todo |
| TQ-28 | Prototypebehuizing in Fusion (STEP en STL in de repo, PETG/ASA) geprint en passend om de elektronica. | WP0.9 | M | todo |
| TQ-29 | Prototype hangt aan de wand; een setpoint via de knop staat binnen 2 s in Apple Home en HA. | WP0.9 | S | todo |
