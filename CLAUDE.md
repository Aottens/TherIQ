# CLAUDE.md — TherIQ

## Wat we bouwen

**TherIQ**: een open-source, modulerende thermostaat voor OpenTherm- én EMS-ketels: volledig lokaal, geen cloud, geen abonnement, tegelijk bedienbaar vanuit Apple Home én Home Assistant via Matter, en als open hardware na te bouwen. Twee units: een **ketelunit** (ESP32-C6, Matter, ketelbus, regeling) naast de ketel en een **kamerunit** (sensor, e-paper, draaiknop) op de plek van de oude thermostaat, verbonden via de bestaande thermostaatkabel.

Lees bij de start van elke sessie **alleen**:

- `docs/STATE.md`: de overdracht: nu, volgende, wat op Aernout wacht.
- `docs/BACKLOG.md`: de taken (TQ-nn) en de voorgestelde builds.

Lees daarna alleen wat de taak nodig heeft:

- `docs/SPEC.md`: volledige specificatie, eisen R1–R10, open beslissingen D0–D8. Bron van waarheid. Lees de secties die de taak raakt, niet het hele bestand.
- `docs/FASE-0.md`: het werkpakket (WP) van de taak, met zijn done-criterium.
- `docs/DECISIONS.md`: genomen beslissingen (ADR-log).
- `docs/IDEAS.md`: ideeën die (nog) niet gepland zijn.

**Huidige fase: 0, bewijs op devkits.** Wat nu aan de beurt is, staat in `docs/STATE.md`.

## Werkwijze: taken, builds, overdracht

Doel: elke sessie en elke subagent werkt met een kleine, volledige context. Overgenomen uit CMDMX, zonder GitHub-review en risk gate (ADR-011).

1. **Taak.** Elk stuk werk is een rij in `docs/BACKLOG.md`: één zin, controleerbaar done-criterium, verwijzing naar WP en eis, grootte S/M/L. XL wordt eerst opgesplitst. Geen code voor werk zonder rij.
2. **Build.** Eén **nieuwe** sessie, één branch, één PR. De planningssessie bouwt zelf niet: ze zet de build klaar als nieuwe sessie met een volledige brief (taken, done-criteria, te lezen bestanden, beslissingen tot nu toe), zodat elke sessie met een schone context begint. Maximaal drie taken die bij elkaar horen (één gedrag over lagen, de ene heeft de andere nodig, of ze raken dezelfde code), waarvan maximaal één M; een L is een eigen build. Apart als één taak los teruggedraaid moet kunnen worden, of als een gate-3-run de rest zou ophouden. De hoofdsessie stelt builds voor op de `Builds:`-regel in de backlog; Aernout keurt ze goed.
3. **Per taak een subagent.** De hoofdsessie schrijft eerst de test die moet slagen (pure logica: host test met het TQ-id in de naam). Een subagent krijgt een korte brief: TQ-id, done-criterium, de falende test, de bestanden die hij moet lezen en wat de vorige taak achterliet. Past de brief niet kort, splits dan de taak. De subagent schrijft de kleinste wijziging die de test groen maakt.
4. **Review en uitleg.** De hoofdsessie draait de tests, leest de diff van elke taak vóór de volgende subagent start, en legt Aernout de code uit (zie *Werken met Aernout*). Eén commit per taak, zodat een taak los terug kan.
5. **Stop.** Blijkt een taak groter dan zijn grootte, of deugt de aanpak niet: lever wat klaar is en zet de rest in de volgende build, met de `Builds:`-regel aangepast in die PR.
6. **Afsluiten in dezelfde PR.** De taakrij wordt `done (datum, #PR)`, of `gate3: <checklist>` als alleen Aernouts run nog ontbreekt. `docs/STATE.md` wordt herschreven (nooit aangevuld, geen afgeronde taken erin). Een beslissing onderweg gaat in `docs/DECISIONS.md`. Aernout merget.
7. **Gate 3.** Werk op hardware of de echte ketel krijgt een checklist in `docs/tests/` (zie `docs/tests/README.md`). Alleen Aernout vinkt af, in een kopie onder `docs/tests/runs/`.
8. **Ideeën.** Een nieuw idee van Aernout gaat in `docs/IDEAS.md`, niet in de backlog of de code, tot zijn fase gepland is.

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
- Hardware in KiCad. Behuizing van het prototype in Fusion (ADR-012): naast het Fusion-bestand altijd STEP en STL in de repo. De CAD-keuze voor de v1-behuizing is nog open.

## Context

- Aernouts ketel: **Nefit TopLine Compact HRC 25/CW4**, spreekt EMS, geen native OpenTherm. OpenTherm testen we tegen een eigen ketelsimulator (`firmware/ketelsim`) en optioneel via een Nefit EMS-OT-converter.
- De live versie van de spec stond in een Claude-doc; deze repo is vanaf nu leidend.
