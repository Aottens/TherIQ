# Fase 0 — bewijs op devkits

Doel: op devkits aantonen dat de architectuur klopt, vóórdat er een eigen print wordt besteld. Fase 0 beantwoordt drie vragen:

1. Werkt één Matter-thermostaat tegelijk, lokaal en stabiel in Apple Home én Home Assistant? (R1, R3)
2. Kunnen we de eigen Nefit-ketel via EMS lezen en aansturen vanuit eigen firmware?
3. Werkt onze OpenTherm-master volledig tegen een ketelsimulator?

Gate fase 0: R1 en R3 gemeten, EMS lezen en schrijven op de eigen ketel, de volledige OpenTherm-berichtenset (SPEC §4) tegen de simulator.

## Uitgangssituatie

- Eigen ketel: **Nefit TopLine Compact HRC 25/CW4**, spreekt **EMS**, geen native OpenTherm (D0).
- Huidige Nefit-thermostaat hangt op dezelfde EMS-bus. Die blijft voorlopig zitten als terugvaloptie.
- Home Assistant is beschikbaar, maar nog niet volledig ingericht. Apple Home met woninghub is in gebruik.

## Veiligheidsregels voor fase 0

- **Eerst alleen lezen.** Niets naar de echte ketel schrijven zonder expliciet akkoord van Aernout per test.
- **Eén regelaar tegelijk.** Nooit twee partijen (bestaande thermostaat, EMS-ESP-gateway, eigen firmware, EMS-OT-converter) tegelijk setpoints laten schrijven.
- **Terugweg vooraf klaar.** Bij elke schrijftest is beschreven hoe de oude situatie in één handeling terugkomt.
- De ketel houdt zijn eigen beveiliging. Wij sturen alleen setpoints.

## Inkooplijst (indicatief)

Controleer actuele beschikbaarheid en of de interfaces op 3,3 V werken met de ESP32-C6.

| Item | Aantal | Voor |
| --- | --- | --- |
| ESP32-C6-devkit (8 MB flash of meer) | 2 | Ketelunit-prototype en ketelsimulator |
| Master-OpenTherm-interface (DIYLESS Master Shield of Ihor Melnyk-adapter) | 1 | OT-master op het prototype |
| Slave-OpenTherm-shield (DIYLESS) | 1 | Ketelsimulator |
| Kant-en-klare EMS-ESP-gateway (BBQKees) | 1 | WP0.4, de ketel leren kennen (D8) |
| EMS-interface voor de C6 | 1 | WP0.5; BBQKees-interfaceprint of zelfbouw naar de open referentieschakeling, keuze in WP0.5 |
| SHT45-breakout | 1 | WP0.3 |
| Nefit EMS-OT-converter (art. 7746901847) | 1, optioneel | WP0.7 |
| Logic analyzer + multimeter | 1 | Timing en rusttoestand meten |

## Werkpakketten

Elk werkpakket is klaar als het done-criterium aantoonbaar is gehaald. Testverslagen gaan in `docs/tests/`.

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

- Kant-en-klare EMS-ESP-gateway aan de ketel; standaardfirmware EMS-ESP.
- Vastleggen: welke waarden de ketel geeft, welke schrijfbaar zijn, hoe de bestaande thermostaat op de bus praat, pollritme en antwoordtijden.
- **Klaar als**: `docs/ketels/nefit-topline-compact-hrc25-cw4.md` bestaat, met een lijst lees-/schrijfwaarden en een ruwe buslog van minimaal 24 uur in `docs/ketels/logs/`.

### WP0.5 — Ketel-API en EMS-driver in eigen firmware

- Ontwerp eerst de abstracte ketel-API (C++-interface): aanvoersetpoint schrijven, CV aan/uit, status, modulatie, aanvoer/retour, druk, buitentemperatuur, storing. OpenTherm en EMS worden implementaties hiervan.
- EMS-driver op de C6, met protocolkennis uit EMS-ESP (GPL-3.0, bronvermelding in de code en in NOTICE).
- Kies hier de EMS-interface voor de C6 en leg de keuze vast als ADR.
- **Klaar als**: eigen firmware leest aanvoer, retour en modulatie van de eigen ketel; daarna, na akkoord van Aernout, schrijft hij een aanvoersetpoint en volgt de ketel dat aantoonbaar (log).

### WP0.6 — OpenTherm-driver en ketelsimulator

- `firmware/ketelsim`: ESP32 met slave-shield die zich als OT-ketel gedraagt, met instelbaar gedrag (welke ID's ondersteund, storingen, traag antwoord, geen antwoord).
- OT-master-driver op de C6 via de RMT-peripheral, achter dezelfde ketel-API.
- **Klaar als**: alle ID's uit SPEC §4 worden uitgewisseld; een "unknown data-ID" wordt onthouden en niet meer gevraagd; met de MCU in reset is de lijnspanning in de rusttoestand (gemeten); een testscript draait de scenario's automatisch.

### WP0.7 — OpenTherm tegen de echte ketel (optioneel)

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
