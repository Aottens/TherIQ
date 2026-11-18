# CLAUDE.md — TherIQ

## Wat we bouwen

**TherIQ**: een open-source, modulerende thermostaat voor OpenTherm- én EMS-ketels: volledig lokaal, geen cloud, geen abonnement, tegelijk bedienbaar vanuit Apple Home én Home Assistant via Matter, en als open hardware na te bouwen. Twee units: een **ketelunit** (ESP32-C6, Matter, ketelbus, regeling) naast de ketel en een **kamerunit** (sensor, e-paper, draaiknop) op de plek van de oude thermostaat, verbonden via de bestaande thermostaatkabel.

Lees bij de start van elke sessie:

- `docs/SPEC.md`: volledige specificatie, eisen R1–R9, open beslissingen D0–D8. Bron van waarheid.
- `docs/FASE-0.md`: het huidige werk, met werkpakketten en done-criteria.
- `docs/DECISIONS.md`: genomen beslissingen (ADR-log).

**Huidige fase: 0, bewijs op devkits.** Begin bij WP0.1, tenzij Aernout anders zegt.

## Harde regels

1. **Geen cloud (R1, R2).** De firmware maakt geen uitgaande internetverbinding, behalve een update-check die de gebruiker zelf start. Geen telemetrie, geen externe API's in de regelketen.
2. **Echte ketel: eerst lezen, schrijven alleen na expliciet akkoord** van Aernout, per test. Beschrijf vooraf hoe de oude situatie terugkomt.
3. **Eén regelaar tegelijk** op de ketelbus. Eén actieve ketelbus per installatie (OpenTherm, EMS of aan/uit), nooit automatisch gekozen.
4. **De ketel houdt zijn eigen beveiliging.** Wij sturen setpoints, we omzeilen niets.
5. **Licenties respecteren.** Firmware is GPL-3.0-or-later. Code uit EMS-ESP (GPL-3.0) mag met bronvermelding in de code en in NOTICE. Schema's van derden pas overnemen nadat de licentie is gecontroleerd en in een ADR vastgelegd.
6. **Spec en beslissingen bijhouden.** Wijkt het werk af van `docs/SPEC.md`, stel dan een spec-wijziging voor en leg het besluit vast in `docs/DECISIONS.md`. Niet stilzwijgend afwijken.

## Werken met Aernout

Aernout is OT/IT-engineer (PLC, SCADA, Ignition) met een UX/HCD-achtergrond. Regeltechniek en veldbussen kent hij; ESP-IDF, esp-matter en embedded C++ zijn voor hem leerterrein. Hij wil de code kunnen lezen, uitleggen en zelf aanpassen.

- **Leg de code uit zodat hij leert**: het waarom, niet alleen het wat. Vooral bij ESP-IDF-, FreeRTOS- en Matter-specifieke keuzes.
- **Probleem eerst, dan oplossing**: stel bij lastige onderdelen eerst de vraag of het probleem, en geef daarna de oplossing.
- **Altijd volledige bestanden**, nooit losse regels of fragmenten om zelf ergens in te voegen.
- **Moeilijke delen stap voor stap**: bouw ze sectie voor sectie op en nodig uit tot vragen tussendoor.
- **Niet versimpelen**: geen speelgoedversies van de code. Hij moet echte code leren lezen.
- **Eén werkpakket tegelijk.** Sluit af met wat er gedaan is, hoe hij het zelf verifieert, en wat het done-criterium was.
- Wees feitelijk. Klopt iets niet in zijn voorstel, zeg dat één keer duidelijk en ga dan door met de inhoud.
- Gebruik nooit de uitdrukking "mea culpa".

## Taal

- Met Aernout en in `docs/`: Nederlands, tot de eerste publieke release (dan vertalen).
- Code, identifiers, code-commentaar en commitberichten: Engels.

## Technische conventies

- ESP-IDF + esp-matter, C++17. Versies vastgepind en gedocumenteerd; build via devcontainer of Docker.
- Eén ESP-IDF-component per verantwoordelijkheid, bijvoorbeeld `boiler_api` (abstract), `ot_driver`, `ems_driver`, `control`, `matter_iface`, `room_bus`, `web_api`.
- Pure logica (regelaar, frame-encoding en -decoding, stooklijn) zonder hardware-afhankelijkheden, met host-side unit tests in CI.
- OpenTherm-timing via de RMT-peripheral, niet bitbangen. EMS via UART, 9600 baud, met break-detectie.
- Sensorwaarden zijn gesynchroniseerd vóórdat de Matter-stack start.
- Hardware in KiCad, behuizing in een open, parametrisch CAD-pakket. Geen bestanden die alleen in betaalde software opengaan.

## Context

- Aernouts ketel: **Nefit TopLine Compact HRC 25/CW4**, spreekt EMS, geen native OpenTherm. OpenTherm testen we tegen een eigen ketelsimulator (`firmware/ketelsim`) en optioneel via een Nefit EMS-OT-converter.
- De live versie van de spec stond in een Claude-doc; deze repo is vanaf nu leidend.
