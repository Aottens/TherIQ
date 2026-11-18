# TherIQ — spec sheet v0.1

Snapshot 18 november 2026. Vanaf nu is deze repo de bron van waarheid; wijzigingen in de spec gaan via een commit op dit bestand plus een ADR in `docs/DECISIONS.md`.

## 1. Doel en harde eisen

Een modulerende thermostaat voor OpenTherm- én EMS-ketels die volledig lokaal werkt, geen enkele functie achter een abonnement zet, tegelijk in Apple Home én Home Assistant te bedienen is, en als open hardware door iedereen na te bouwen is.

Ontwerpprincipe: de fabrikant is de gebruiker. Alles wat de thermostaat kan, staat in de open firmware; er is geen server die ooit uit kan.

| ID | Eis | Verificatie |
| --- | --- | --- |
| R1 | Volledig functioneel zonder internet of cloud | Router WAN-kabel eruit: regeling, Apple Home en HA blijven werken |
| R2 | Geen functie afhankelijk van een betaald account | Code-review: geen externe endpoints in de regelketen |
| R3 | Gelijktijdig lokaal bedienbaar vanuit Apple Home en Home Assistant | Setpoint wijzigen in de ene app is binnen 2 s zichtbaar in de andere |
| R4 | Modulerende aansturing via OpenTherm én EMS (Bosch-familie: Nefit, Buderus, Junkers, Worcester); aan/uit-relais als fallback voor de rest | Test op een EMS-ketel, een OpenTherm-ketel (of simulator, zie 4) en een aan/uit-ketel |
| R5 | Blijft regelen als wifi, HA of de Apple-woninghub wegvalt | Netwerk uit: kamertemperatuur blijft binnen ±0,5 °C van het setpoint |
| R6 | Hergebruikt de bestaande 2-draads thermostaatkabel; geen nieuwe kabels door het huis | Installatie zonder hak- en breekwerk |
| R7 | Geen netspanning (230 V) op de eigen printplaten | Schema-review |
| R8 | Na te bouwen via standaard PCB-assemblage (bijv. JLCPCB) en een 3D-geprinte behuizing | Externe bouwer bouwt hem uit de repo zonder hulp |
| R9 | Kamertemperatuurmeting ±0,2 °C na compensatie van eigen opwarming | Vergelijking met een referentiethermometer, 24 u |
| R10 | Materiaalkosten ≤ € 100 per set (ketelunit + kamerunit: onderdelen, JLCPCB-print en -assemblage; excl. filament, USB-adapter en verzending) | BOM-export uit CI, prijspeil JLCPCB bij 5 stuks |

Buiten scope voor v1: zoneregeling met radiatorknoppen, koeling, warmtepomp-specifieke functies, sanitair-warmwaterprogrammering anders dan aan/uit.

## 2. Systeemarchitectuur

Het systeem bestaat uit twee eigen units: een ketelunit naast de ketel en een kamerunit op de plek van de oude thermostaat. De bestaande thermostaatkabel wordt de kamerbus tussen die twee.

```
                 ┌──────────────────────┐   ┌──────────────────────┐
                 │ Apple Home           │   │ Home Assistant       │
                 │ via woninghub        │   │ Matter Server        │
                 │ (HomePod/Apple TV)   │   │ + optioneel MQTT     │
                 └──────────▲───────────┘   └──────────▲───────────┘
                Matter (wifi)│                         │Matter + MQTT
┌──────────────────┐    ┌────▼─────────────────────────▼───┐    ┌─────────────────────┐
│ Ketel            │◄──►│ Ketelunit                        │◄──►│ Kamerunit           │
│ beveiliging      │ OT │ ESP32-C6, Matter                 │bus │ SHT45, e-paper      │
│ blijft in ketel  │/EMS│ OT/EMS, regeling, aan/uit-relais │    │ draaiknop           │
│ OT, EMS, aan/uit │kort│                                  │best│ gevoed via kamerbus │
└──────────────────┘    └────────────────▲─────────────────┘kabel└───────────────────┘
                                         │ 5 V
                                 ┌───────┴───────┐
                                 │ USB-C-adapter │
                                 └───────────────┘
```

De ketelunit praat OpenTherm of EMS met de ketel over een nieuw, kort draadje en Matter met Apple Home en HA. De kamerunit hangt aan de bestaande kabel, die voeding en data draagt.

Waarom gesplitst: de OpenTherm-bus levert te weinig vermogen voor wifi (zie 4), en bij de oude thermostaatplek zit zelden een stopcontact. Naast de ketel meestal wel. Zo blijft R6 overeind: geen nieuwe kabels door het huis.

Waarom alle intelligentie in de ketelunit: valt de kamerunit uit, dan regelt de ketelunit op de stooklijn door (zie 7). Het huis koelt nooit af door een kapotte display of een losse draad.

## 3. Integratie: Matter als primaire interface

Besluit (voorstel): de thermostaat spreekt **Matter over wifi**, niet HomeSpan. Alleen Matter haalt R3: één apparaat dat tegelijk, lokaal en los van elkaar in Apple Home en Home Assistant zit. Dat heet multi-admin ([lazyadmin](https://lazyadmin.nl/smart-home/home-assistant-matter/), [HA-forum](https://community.home-assistant.io/t/matter-pairing-fails-with-eve-thermostats-what-am-i-missing/949430)).

HomeSpan spreekt het HomeKit Accessory Protocol. Een HAP-apparaat hoort bij één ecosysteem tegelijk; HA zou hem via HomeKit Controller moeten overnemen uit Apple Home. Dat botst met R3.

| Optie | Apple Home | Home Assistant | Firmware-stacks | Oordeel |
| --- | --- | --- | --- | --- |
| Matter (esp-matter) | Native, lokaal | Matter-integratie, tegelijk | 1 | Voorstel |
| HomeSpan + MQTT | Native via HAP | MQTT-discovery, tegelijk | 2, eigen state-sync | Terugvaloptie |
| Alleen HomeSpan | Native via HAP | Alleen door Apple Home los te koppelen | 1 | Valt af (R3) |

### Interfaces van het apparaat

- **Matter, device type Thermostat**: Thermostat-cluster (modus Heat/Off, setpoint, kamertemperatuur) plus luchtvochtigheid. Dit is wat Apple Home en de basisbediening in HA gebruiken.
- **MQTT met HA-discovery (optioneel, standaard uit)**: de ketel-diagnostiek die Matter niet kent: modulatie %, aanvoer- en retourtemperatuur, waterdruk, storingscodes, branderuren.
- **Lokale web-UI + REST-API**: installatie, stooklijn, PID-parameters, ketelbus-log, firmware-update. Geen account, geen app nodig.

### Bekende valkuilen

- **Certificering**: zonder CSA-certificering draait het apparaat op een test-vendor-ID (esp-matter gebruikt standaard 0xFFF1, [Espressif](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)). Apple Home accepteert dat, maar toont "Uncertified Accessory" ([Apple-forum](https://developer.apple.com/forums/thread/844870)). Voor een open-sourceproject is dat acceptabel; certificering is een latere keuze (D7).
- **State vóór commissioning**: een thermostaat die tijdens het koppelen nog null-waarden meldt, kan door ecosystemen als "niet ondersteund" worden onthouden. Sensorwaarden moeten gesynchroniseerd zijn voordat de Matter-stack start ([praktijkverslag ESP32-C6-thermostaat](https://joshsmind.pages.dev/blog/three-days-of-not-supported-a-matter-thermostat-debugging-adventure)).
- **Ontkoppelen**: Apple Home kan een fabric niet geforceerd intrekken ([HA-forum](https://community.home-assistant.io/t/matter-thread-and-why-just-reset-it-is-a-myth/976430)). Het apparaat krijgt daarom een fysieke fabrieksreset (knop 10 s) en een reset via de web-UI.
- **HA-kant**: Matter vereist in HA de Matter Server en IPv6 op de netwerkinterface ([lazyadmin](https://lazyadmin.nl/smart-home/home-assistant-matter/)).

Thread: de gekozen chip kan het (zie 5), maar v1 blijft op wifi omdat de ketelunit vaste voeding heeft. Thread wordt een firmware-optie, geen hardwarewijziging.

## 4. Ketelinterfaces: OpenTherm en EMS

De ketelunit spreekt twee ketelbussen: OpenTherm (Remeha, Intergas en vele anderen) en EMS (de Bosch-familie, waaronder Nefit). De regeling praat met een abstracte ketel-API (setpoint, modulatie, temperaturen, storing); OpenTherm en EMS zijn drivers daaronder. Bij OpenTherm is de ketelunit master: hij stuurt elke seconde een bericht en de ketel antwoordt. De interface wordt galvanisch gescheiden van de microcontroller en moet bij een vastgelopen of stroomloze MCU in de rusttoestand blijven.

### OpenTherm: fysieke laag

| Parameter | Waarde | Bron |
| --- | --- | --- |
| Kabel | 2 draden, polariteitsongevoelig, max. 50 m / 2 × 5 Ω | [Wikipedia](https://en.wikipedia.org/wiki/OpenTherm) |
| Master → ketel | spanningsmodulatie: laag < 9 V, hoog 15–18 V (ketel levert ca. 24 V open klem) | [meetverslag](https://antenna-handbook.blogspot.com/2024/10/Opentherm%20Monitor%20Circuit%20.html) |
| Ketel → master | stroommodulatie: laag 5–9 mA, hoog 17–23 mA | [meetverslag](https://antenna-handbook.blogspot.com/2024/10/Opentherm%20Monitor%20Circuit%20.html) |
| Codering | Manchester, 1 ms per bit, frame van 32 bits | [Wikipedia](https://en.wikipedia.org/wiki/OpenTherm) |
| Interval | master stuurt minimaal elke seconde | [Wikipedia](https://en.wikipedia.org/wiki/OpenTherm) |
| Rusttoestand | lijnspanning laag (< 7 V); niet hoog blijven hangen | [schakelingdiscussie](https://forum.allaboutcircuits.com/threads/opentherm-improved-idle-safe-circuit-design.202983/) |
| Vermogen uit de bus | standaard minimaal; met Smart Power 136 mW (medium) of 255 mW (high) | [Wikipedia](https://en.wikipedia.org/wiki/OpenTherm) |

Het busvermogen is te laag voor wifi. De ketelunit krijgt daarom eigen voeding (zie 5); de bus voedt niets.

Bestaande open schakelingen dienen als referentie bij het ontwerp: Ihor Melnyk's OpenTherm Adapter en de DIYLESS Master Shield ([esphome-opentherm](https://github.com/olegtarasov/esphome-opentherm)). Licenties van die schema's worden gecontroleerd voordat er iets uit wordt overgenomen.

### OpenTherm: berichten (minimale set v1)

| Data-ID | Inhoud | Richting |
| --- | --- | --- |
| 0 | Status: CH aan/uit, DHW aan, vlam, storing | lezen/schrijven |
| 1 | Aanvoertemperatuur-setpoint (de regelgrootheid) | schrijven |
| 3 | Ketelconfiguratie | lezen |
| 5 | Storingsvlaggen en OEM-code | lezen |
| 14 | Maximale modulatie | schrijven |
| 17 | Actuele modulatie | lezen |
| 18 | Waterdruk CV | lezen |
| 24 / 16 | Kamertemperatuur / kamersetpoint (informatief naar ketel) | schrijven |
| 25 / 28 | Aanvoer- / retourtemperatuur | lezen |
| 27 | Buitentemperatuur (als de ketel een buitenvoeler heeft) | lezen |
| 56 / 57 | Warmwater-setpoint / max. CV-setpoint | lezen/schrijven |
| 124 / 125 | OpenTherm-versie master / ketel | schrijven / lezen |

Niet-ondersteunde ID's (ketel antwoordt "unknown data-ID") worden per ketel onthouden en niet meer gevraagd.

### EMS (Bosch-familie)

Bij EMS is de rol omgekeerd: de ketel is busmaster en pollt elke seconde alle adressen; de thermostaat antwoordt als hij aan de beurt is ([EMS-ESP](https://backiee.wasmer.app/https_emsesp_org/EMS-bus)). Nefit en Junkers noemen het ook de "2-draadsbus" ([EMS-Wiki](https://github.com/bbqkees/EMS-Wiki)).

- **Fysieke laag**: seriële data op 9600 baud, elk bericht afgesloten met een break ([ems_bus](https://github.com/CreaValix/ems_bus)). Spanningsniveaus worden overgenomen uit de gepubliceerde EMS-ESP-referentieschakeling ([EMS-circuit](https://docs-7m8.pages.dev/EMS-Circuit)) en nagemeten op de eigen ketel.
- **Protocolkennis**: uit EMS-ESP, open-source ESP32-firmware voor Bosch, Buderus, Nefit, Junkers en Worcester, die al een thermostaat kan simuleren ([GitHub](https://github.com/emsesp/EMS-ESP32)). EMS-ESP is GPL-3.0, net als deze firmware (zie 9), dus hergebruik met bronvermelding mag.
- **Hardware**: een eigen EMS-interface op de ketelunit, naast de OpenTherm-interface. Elke bus krijgt een eigen klemmenblok.
- **Busmaster-gevolg**: de ketelunit kan bij EMS de pollfrequentie niet zelf bepalen. De regeling moet tegen wisselende antwoordtijden kunnen.

Alternatief voor EMS-ketels: Nefits EMS-OT-converter (art. 7746901847) ertussen, dan praat de ketelunit gewoon OpenTherm ([CV Totaal](https://www.cvtotaal.nl/amfile/file/download/file/115/product/27311), [Theben](https://www.theben.de/ocsmedia/optimized/full/o4312v67_KNXOTBox_Produktinformationen.PDF)). Dat kost een extra fabrikantkastje en je ziet alleen wat de converter doorgeeft; de directe EMS-route is daarom de standaard.

### Testen zonder OpenTherm-ketel

1. **Ketelsimulator**: een tweede ESP32 met een slave-OpenTherm-shield, dat bedoeld is voor ketelsimulatie ([DIYLESS](https://diyless-old.duckdns.org/product/slave-opentherm-shield)), en de MIT-gelicenseerde OpenThermPlus-library die master, slave en gateway kan ([Arduino Libraries](https://www.arduinolibraries.info/libraries/open-therm-plus)). Deze simulator wordt ook de vaste hardware-in-the-loop-testbank voor CI.
2. **Echte OpenTherm-slave**: de EMS-OT-converter aan de eigen Nefit-ketel. Zo test de OpenTherm-master tegen een fabrikantimplementatie. Nooit tegelijk met de directe EMS-route laten regelen.
3. **Community**: bouwers met een Remeha, Intergas of andere OpenTherm-ketel testen en melden in de ketellijst (zie 9).

### Aan/uit-fallback

Voor ketels zonder OpenTherm of EMS krijgt de ketelunit een apart potentiaalvrij contact (relais of PhotoMOS) op een eigen klemmenblok. De ketelbus (OpenTherm, EMS of aan/uit) wordt bij installatie in de web-UI gekozen, nooit automatisch, en er is altijd maar één actief.

### Later (v2)

Gateway-modus: de unit tussen een bestaande OpenTherm-thermostaat en de ketel, die het verkeer doorgeeft en meeleest. Handig als overgangsfase, maar het vraagt een tweede interface (slave-kant) en valt buiten v1.

## 5. Hardware: twee printplaten

Twee PCB's: een **ketelunit** (netwerk, regeling, ketelbus) en een **kamerunit** (meten, tonen, bedienen). Alle intelligentie zit in de ketelunit; de kamerunit is bewust dom en zuinig, zodat hij over de bestaande thermostaatdraad gevoed kan worden.

### Ketelunit

| Functie | Keuze (voorstel) | Waarom |
| --- | --- | --- |
| MCU + radio | ESP32-C6-WROOM-1-N8 | Wifi 6, BLE 5 en Thread op één module ([Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf)); 8 MB flash voor Matter + twee OTA-partities; voorgecertificeerde module scheelt radiokeuring |
| Voeding | USB-C, 5 V, externe gecertificeerde adapter | Geen 230 V op de print (R7) |
| OpenTherm | Eigen master-interface, galvanisch gescheiden (optocouplers), bruggelijkrichter, TVS | Zie 4; rusttoestand ook bij MCU in reset |
| EMS | Eigen EMS-interface naar de open EMS-ESP-referentieschakeling, eigen klemmenblok | Bosch-familie (o.a. Nefit) zonder converter; zie 4 |
| Aan/uit-uitgang | PhotoMOS, potentiaalvrij, eigen klemmenblok | Fallback voor ketels zonder OpenTherm of EMS |
| Kamerbus | Voeding + data naar de kamerunit over de bestaande 2-draads kabel | Zie D2 |
| Uitbreiding | 1-Wire-klem (aanvoer- of buitenvoeler), I²C-connector (Qwiic) | Eigen sensoren zonder nieuwe print |
| Bediening/status | Resetknop (10 s = fabrieksreset), RGB-status-led | Valkuil ontkoppelen, zie 3 |
| Debug/flashen | Native USB van de C6 via dezelfde USB-C | Geen aparte programmer nodig |
| Montage | Wandmontage naast de ketel, optioneel DIN-rail | Ketelkast of meterkast |

### Kamerunit

| Functie | Keuze (voorstel) | Waarom |
| --- | --- | --- |
| Temperatuur + RV | Sensirion SHT45: ±0,1 °C, ±1,0 %RV typisch ([Sensirion](https://sensirion.com/products/catalog/SHT45)) | Nauwkeurigheid R9 is dan haalbaar, mits de print zelf niet opwarmt |
| Display | E-paper, ca. 2,9" | Vrijwel geen eigen warmte, leesbaar zonder verlichting, nul vermogen in rust |
| Invoer | Draaiknop (encoder met drukfunctie) | Setpoint zonder app; zie 6 |
| MCU | Zuinige MCU zonder radio | Radio is niet nodig; keuze in D3 |
| Voeding | Uit de kamerbus | R6: geen batterijen, geen extra kabel |
| Aanwezigheid (optioneel) | Footprint voor een PIR- of mmWave-module | Display wekken; later aanwezigheidsdetectie |

Vermogensbudget kamerunit: doel ≤ 150 mW gemiddeld. Dit getal stuurt de keuze van display, MCU en kamerbus.

### Layoutregels

- **Thermisch (kamerunit)**: SHT45 op een uitgezaagd schiereiland aan de onderrand, gescheiden van MCU en spanningsregelaar door sleuven; geen koperen vlakken onder de sensor. Luchtinlaat onder, uitlaat boven in de behuizing. Restopwarming wordt in de firmware gecompenseerd en gekalibreerd tegen een referentie (R9).
- **Radio (ketelunit)**: antenne-keepout van de module aan de printrand, geen koper of metaal eronder; de behuizing rond de antenne zonder metaal.
- **Velddraden**: elke klem krijgt TVS-bescherming, en ketelbuszijde en MCU-zijde houden een zichtbare scheidingsspleet.
- **Klemmen**: steekbare schroefklemmen, gelabeld op print en behuizing.
- **Bouwbaarheid**: alle componenten verkrijgbaar bij de gangbare assemblagepartijen; geen BGA op de eigen print (R8).

## 6. Behuizing en bediening

De kamerunit moet zonder app, zonder uitleg en door elke huisgenoot te bedienen zijn: draaien is warmer of kouder, drukken is de modus. Al het andere zit in Apple Home, HA of de web-UI.

### Kamerunit

- **Formaat**: minimaal 86 × 86 mm voorvlak, zodat schroefgaten en verfranden van de oude thermostaat bedekt zijn.
- **Tweedelig**: een wandplaat met de klemmen wordt vastgeschroefd en bedraad; het voorstuk klikt erop met een contactstrip. Het voorstuk is te vervangen of te updaten zonder aan de bedrading te komen.
- **Luchtstroom**: sleuven onder en boven; de sensorkamer is met een binnenwand gescheiden van de elektronica (zie de layoutregels in 5).
- **Materiaal**: PETG of ASA (3D-print), geen PLA: dat wordt zacht bij warmtebronnen in de buurt.

### Ketelunit

- Wandmontage met trekontlasting op elke kabelinvoer, klemmen afgeschermd achter een deksel.
- Rond de antenne geen metaal; de unit niet in een metalen kast monteren (montage-instructie).
- Label met de Matter-QR-code op de behuizing én in de documentatie, zodat opnieuw koppelen altijd kan.

### Bedieningsregels

1. **Draaien** zet het setpoint in stappen van 0,5 °C; het nieuwe getal staat direct op het scherm (gedeeltelijke e-paper-verversing).
2. **Drukken** opent één kort menu: Verwarmen, Uit, Afwezig, Boost. Niet dieper dan één niveau.
3. **Rustscherm**: kamertemperatuur groot, setpoint klein, vlam-icoon als de brander brandt, netwerkstatus als klein symbool.
4. **Handmatige wijziging** geldt tot het volgende schakelmoment van het schema; daarna neemt het schema het weer over. Het scherm toont tot wanneer ("tot 22:30").
5. **Storing**: ketelstoring met code én leesbare tekst op het scherm, en een melding in Apple Home en HA.
6. **Zonder netwerk** blijven draaien, drukken en het schema gewoon werken (R5).
7. **Kinderslot** optioneel, in te stellen in de web-UI.

## 7. Firmware en regeling

De regeling draait volledig op de ketelunit: een PI-regelaar op kamertemperatuur bovenop een stooklijn, met als uitgang het aanvoer-setpoint (OT-ID 1, of het EMS-equivalent). De ID's in deze sectie zijn OpenTherm; de EMS-driver levert dezelfde grootheden via de ketel-API. Valt de kamermeting weg, dan blijft de stooklijn alleen over en blijft het huis warm.

### Stack

- ESP-IDF + esp-matter (C++), FreeRTOS-taken per verantwoordelijkheid: ketelbus (OpenTherm of EMS), regeling, kamerbus, Matter, web/MQTT.
- OpenTherm-Manchester via de RMT-peripheral van de C6 (in hardware, geen bitbangen), zodat wifi-pieken de 1 ms-bittiming niet verstoren.
- EMS via UART op 9600 baud met break-detectie.
- Configuratie in NVS; elke parameter ook leesbaar en schrijfbaar via de REST-API.

### Regelwet

```
T_aanvoer = clamp( T_stooklijn(T_buiten) + Kp·e + Ki·∫e dt , T_min , T_max ),   e = T_set − T_kamer
```

- **Stooklijn** (feedforward): aanvoertemperatuur als functie van buitentemperatuur, uit de buitenvoeler van de ketel (OT-ID 27), een eigen 1-Wire-voeler of, als laatste optie, een waarde uit HA.
- **PI** (feedback): corrigeert op de kamerfout, met anti-windup op de clamp.
- **Begrenzing**: T_max niet boven het maximum dat de ketel meldt (OT-ID 57); maximale modulatie instelbaar (OT-ID 14).
- **Doel**: zo laag mogelijke aanvoertemperatuur, zodat een HR-ketel blijft condenseren. De web-UI toont aanvoer, retour en modulatie als grafiek om de stooklijn af te stellen.
- **Autotune** (v2): stooklijn en PI-parameters leren uit een week data.

### Modi en schema

- **Modi**: Verwarmen, Uit (met vorstbeveiliging), Afwezig, Boost (tijdelijk hoger setpoint).
- **Schema**: opgeslagen op het apparaat, bewerkbaar in de web-UI en HA. Apple Home kent geen thermostaatschema's; daar loopt het via automatiseringen of het interne schema.
- **Tijd**: via SNTP op het lokale netwerk (router of HA); zonder netwerk draait de interne klok door. Een RTC-chip met supercap is optioneel (D4).

### Fail-safe gedrag

| Gebeurtenis | Reactie | Melding |
| --- | --- | --- |
| Kamerunit valt weg | Regelen op stooklijn met laatste setpoint | Apple Home, HA, status-led |
| Netwerk valt weg | Alles lokaal door: schema, knop, regeling | Status-led, icoon op kamerunit |
| Ketelbus-communicatie valt weg | Ketel valt terug op eigen gedrag (per ketel testen, D5) | Alle kanalen + scherm |
| Kamersensor implausibel (sprong, buiten bereik) | Sensor negeren, stooklijn | Alle kanalen |
| Firmware hangt | Watchdog-reset; OT-lijn in rusttoestand (zie 4) | Teller in log |
| Ketelstoring (OT-ID 5 / EMS-storingscode) | Niets forceren, alleen doorgeven | Code + tekst op scherm en in apps |

### Updates en beveiliging

- **OTA**: twee partities, ondertekende images, automatische rollback als de nieuwe versie niet opstart.
- **Geen stille verbindingen**: de firmware maakt geen uitgaande verbinding naar internet tenzij de gebruiker zelf een update laat zoeken (R1, R2).
- **Web-UI**: wachtwoord instellen bij eerste installatie; HTTPS met een lokaal gegenereerd certificaat.

## 8. Veiligheid en regelgeving

Zolang het project ontwerpen publiceert die mensen zelf bouwen, is de last licht. Zodra er kits of kant-en-klare units worden verkocht, wordt de verkoper fabrikant en gelden CE-eisen én de nieuwe Europese cyberregels. Het ontwerp houdt daar vanaf dag 1 rekening mee.

### Wat het ontwerp zelf afvangt

- **Geen netspanning**: alleen 5 V uit een gecertificeerde adapter en laagspanning op de ketelbus en kamerbus (R7).
- **Ketelbeveiliging blijft in de ketel**: de thermostaat stuurt alleen setpoints en aan/uit; branderbeveiliging, maximale watertemperatuur en drukbewaking blijven bij de ketel.
- **Vorstbeveiliging** ook in de modus Uit.
- **Voorgecertificeerde radiomodule**: scheelt een groot deel van de radiotests, maar vervangt de keuring van het eindproduct niet.

### Regels bij verkoop (EU)

| Regel | Wat | Geldt vanaf |
| --- | --- | --- |
| RED 2014/53/EU | Radio-, EMC- en veiligheidseisen voor apparaten met radio (CE) | Geldt al |
| RED-gedelegeerde verordening 2022/30 (EN 18031) | Cybersecurity-eisen voor radioapparatuur met internetverbinding | 1 augustus 2025 ([Sorena](https://sorena-io.pages.dev/artifacts/eu/cyber-resilience-act/cra-vs-red-cybersecurity-delegated-act.md)) |
| Cyber Resilience Act (2024/2847) | Meldplicht voor actief misbruikte kwetsbaarheden | 11 september 2026 ([noze](https://www.noze.it/en/insights/cra-commission-guidance/)) |
| Cyber Resilience Act (2024/2847) | Volledige eisen, o.a. SBOM en beveiligingsupdates | 11 december 2027 ([Europese Commissie](https://digital-strategy.ec.europa.eu/en/factpages/cyber-resilience-act-implementation)) |
| RoHS en WEEE | Stoffen in componenten; terugname van elektronica | Geldt al |

Geld vragen voor de software, of updates afhankelijk maken van donaties, kan het project onder de CRA tot "op de markt gebracht" maken ([Thomas Murray](https://cyber.thomasmurray.com/insights/eu-cyber-resilience-act-commission-publishes-implementation-guidance)). Wie het project alleen structureel ondersteunt zonder te verkopen, kan onder de lichtere rol van "open-source steward" vallen ([noze](https://www.noze.it/en/insights/cra-commission-guidance/)).

Voor v1 betekent dit: firmware levert vanaf het begin een SBOM, een gedocumenteerd updatebeleid en een security-contact (SECURITY.md). Dit is geen juridisch advies; vóór verkoop van kits volgt een toets bij een keuringsinstantie of specialist.

## 9. Open source: licenties en repo

Voorstel: copyleft op alles wat het project zelf maakt, zodat niemand het ontwerp kan sluiten en er een abonnement omheen kan bouwen.

| Onderdeel | Licentie (voorstel) | Effect |
| --- | --- | --- |
| Hardware (schema, PCB, behuizing) | CERN-OHL-S-2.0 | Afgeleide hardware moet open blijven |
| Firmware | GPL-3.0-or-later | Afgeleide firmware moet open blijven; de Apache-2.0-bibliotheken eronder (ESP-IDF, esp-matter) zijn daarmee te combineren, en EMS-ESP-code (ook GPL-3.0) mag worden hergebruikt |
| Documentatie | CC BY-SA 4.0 | Vrij te delen en te bewerken, met naamsvermelding |

### Repo-structuur

```
/hardware/ketelunit      KiCad-project
/hardware/kamerunit      KiCad-project
/enclosure               parametrisch CAD-model + STEP/STL
/firmware/ketelunit      ESP-IDF + esp-matter
/firmware/kamerunit      firmware kamerunit
/firmware/ketelsim       OpenTherm-ketelsimulator (testbank)
/docs                    spec, beslissingen, installatie, ketellijst
/tools                   ketelbus-logger, kalibratiescript sensor
SECURITY.md              meldpunt kwetsbaarheden
```

- **Volledig open toolchain**: KiCad voor de printen, een open CAD-pakket voor de behuizing. Geen bestand dat alleen in betaalde software opent.
- **CI**: elke commit bouwt de firmware, genereert de SBOM en exporteert Gerbers, BOM en pick-and-place, zodat iedereen een printbare release kan bestellen.
- **Ketellijst**: een door de community bijgehouden tabel met geteste ketels: merk en type, ketelbus, protocolversie, ondersteunde berichten en gedrag bij communicatieverlies (D5).

## 10. Fasering en open beslissingen

Fase 0 gaat eerst, op devkits met kant-en-klare EMS- en OpenTherm-interfaces, en test beide ketelbussen meteen: EMS direct op de eigen Nefit-ketel, OpenTherm via de ketelsimulator en de EMS-OT-converter (zie 4). Fase 0 bewijst ook Matter, want als dat niet tegelijk goed werkt in Apple Home én HA, verandert de architectuur, en dan wil je nog geen eigen print hebben laten maken.

| Fase | Inhoud | Gate |
| --- | --- | --- |
| 0 · Bewijs op devkit | ESP32-C6-devkit, EMS- en OT-interface, Matter in Apple Home en HA tegelijk, EMS-ketel + OT-simulator | R1 + R3 aangetoond |
| 1 · Ketelunit rev A | Eigen PCB, OT-/EMS-interface, relais, regeling + fail-safe; kamersensor nog los | 2 weken stabiel stoken |
| 2 · Kamerunit rev A | Kamerbus (D2), e-paper, draaiknop, behuizing, wandplaat, sensorkalibratie | R5, R6, R9 gemeten |
| 3 · Publicatie | Docs + CI-releases, ketellijst, externe bouwers | R8: bouw zonder hulp |

### Mijlpaal 31 december 2026 (ADR-008)

Werkend op devkits, in huis in gebruik:

- Ketelunit op een ESP32-C6-devkit regelt de eigen Nefit via EMS, bedienbaar in Apple Home én HA, met schema (o.a. nachtverlaging).
- Kamerunit-prototype aan de wand: devkit + SHT45 + e-paper + draaiknop in een 3D-geprinte behuizing.
- Eigen PCB's (rev A) mogen in ontwerp of besteld zijn, maar hoeven niet af.

Dit valt over fase 0 heen en loopt deels vooruit op fase 2. De gates per fase blijven gelden.

Elke fase sluit af met een gate die aan de eisen uit 1 hangt; de volgende fase start pas als die gemeten is. Uitwerking van fase 0: `docs/FASE-0.md`.

### Open beslissingen

| ID | Vraag | Opties | Voorstel |
| --- | --- | --- | --- |
| D0 | Welke ketelbus spreekt de eigen ketel? | Beantwoord: Nefit TopLine Compact HRC 25/CW4 spreekt EMS, geen native OpenTherm | EMS direct testen op de eigen ketel; OpenTherm via simulator en EMS-OT-converter |
| D1 | Integratieprotocol | Matter / HomeSpan + MQTT | Matter (zie 3) |
| D2 | Fysieke laag kamerbus | Eigen laag naar OpenTherm-principe / bestaande 2-draads standaard / draadloze kamerunit op batterij | Uitzoeken in fase 1; eis: ≥ 150 mW naar de kamerunit, polariteitsongevoelig, 50 m |
| D3 | MCU kamerunit | Zuinige MCU zonder radio / ESP32-C6 met radio uit | Volgt uit D2 en het vermogensbudget |
| D4 | RTC met supercap op ketelunit | Ja / nee | Ja, schema blijft dan correct na stroomuitval zonder netwerk |
| D5 | Gedrag van de ketel bij communicatieverlies | Per ketel en per bus verschillend | Testen op eigen ketel (EMS) en simulator (OT); vastleggen in de ketellijst |
| D6 | Projectnaam | Beantwoord: **TherIQ** (ADR-007) | — |
| D7 | Matter-certificering | Ja (kosten, lidmaatschap) / nee ("Uncertified" in Apple Home) | Nee voor v1; heroverwegen bij verkoop van kits |
| D8 | EMS-hardware in fase 0 | Kant-en-klare EMS-ESP-gateway / zelfbouw naar de open referentieschakeling | Beantwoord (ADR-013): BBQKees-interfacebord met EMS-ESP op een eigen ESP32 om de ketel te leren kennen; daarna hetzelfde bord aan de C6 |

## Bronnen

- [OpenTherm – Wikipedia](https://en.wikipedia.org/wiki/OpenTherm)
- [OpenTherm monitor circuit – meetverslag signaalniveaus](https://antenna-handbook.blogspot.com/2024/10/Opentherm%20Monitor%20Circuit%20.html)
- [OpenTherm idle-safe circuit design – All About Circuits](https://forum.allaboutcircuits.com/threads/opentherm-improved-idle-safe-circuit-design.202983/)
- [esphome-opentherm (referentie-interfaces)](https://github.com/olegtarasov/esphome-opentherm)
- [Home Assistant Matter-setup – lazyadmin](https://lazyadmin.nl/smart-home/home-assistant-matter/)
- [Matter multi-admin met Apple Home en HA – HA-forum](https://community.home-assistant.io/t/matter-pairing-fails-with-eve-thermostats-what-am-i-missing/949430)
- [Matter/Thread-ontkoppelproblemen – HA-forum](https://community.home-assistant.io/t/matter-thread-and-why-just-reset-it-is-a-myth/976430)
- [Uncertified Accessory in Apple Home – Apple Developer Forums](https://developer.apple.com/forums/thread/844870)
- [esp-matter: ontwikkelen met de SDK – Espressif](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
- [DIY Matter-thermostaat op ESP32-C6 – praktijkverslag](https://joshsmind.pages.dev/blog/three-days-of-not-supported-a-matter-thermostat-debugging-adventure)
- [ESP32-C6-WROOM-1 datasheet – Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf)
- [SHT45 – Sensirion](https://sensirion.com/products/catalog/SHT45)
- [CRA vs RED-gedelegeerde verordening – Sorena](https://sorena-io.pages.dev/artifacts/eu/cyber-resilience-act/cra-vs-red-cybersecurity-delegated-act.md)
- [CRA-tijdlijn – Europese Commissie](https://digital-strategy.ec.europa.eu/en/factpages/cyber-resilience-act-implementation)
- [CRA-richtsnoeren – noze](https://www.noze.it/en/insights/cra-commission-guidance/)
- [CRA en open source – Thomas Murray](https://cyber.thomasmurray.com/insights/eu-cyber-resilience-act-commission-publishes-implementation-guidance)
- [EMS-ESP32 – GitHub](https://github.com/emsesp/EMS-ESP32)
- [EMS-bus uitgelegd – EMS-ESP](https://backiee.wasmer.app/https_emsesp_org/EMS-bus)
- [EMS-ESP referentieschakeling](https://docs-7m8.pages.dev/EMS-Circuit)
- [EMS-Wiki – BBQKees](https://github.com/bbqkees/EMS-Wiki)
- [ems_bus protocolstack – CreaValix](https://github.com/CreaValix/ems_bus)
- [Aansluittabel thermostaten – CV Totaal](https://www.cvtotaal.nl/amfile/file/download/file/115/product/27311)
- [KNX-OT-Box compatibiliteitslijst – Theben](https://www.theben.de/ocsmedia/optimized/full/o4312v67_KNXOTBox_Produktinformationen.PDF)
- [Slave OpenTherm Shield – DIYLESS](https://diyless-old.duckdns.org/product/slave-opentherm-shield)
- [OpenThermPlus-library](https://www.arduinolibraries.info/libraries/open-therm-plus)
