# Checklist: EMS-ESP alleen lezend aan de Nefit, 24 u buslog (TQ-09, WP0.4)

Gate 3: alleen Aernout vinkt af. Kopieer naar `docs/tests/runs/<datum>-TQ-09.md` voor een run. Doel: een ruwe buslog
van minimaal 24 uur van de Nefit TopLine Compact HRC 25/CW4 in `docs/ketels/logs/`, zonder dat er iets naar de ketel
wordt geschreven (CLAUDE.md, harde regels 2 en 3). De bestaande Nefit-thermostaat blijft op de bus en blijft de enige
regelaar; EMS-ESP regelt niets.

## Terugweg
- **Stekker eruit = oude situatie.** EMS-ESP hangt parallel aan de bus via de service-jack van de ketel. Jack eruit,
  en de bus is weer precies zoals hij was: aan de ketel en de thermostaat is niets losgedraaid of ingesteld.
- Alleen als de ketel geen service-jack heeft (zie *Aansluiten*): de twee extra draden hangen parallel op de
  EMS-klemmen; die twee draden eraf = oude situatie. Er gaat geen bestaande draad los.
- Wanneer meteen terug: thermostaatdisplay leeg, storingscode op de ketel, of de ketel gedraagt zich anders dan
  vóór het aansluiten. Stekker eruit, binnen een minuut moeten thermostaat en ketel weer normaal zijn. Zo niet: ketel
  één keer uit en aan met de hoofdschakelaar.
- EMS-ESP schrijft in deze checklist niets naar de ketel (fase 1: zender helemaal uit; fase 2: alleen leesvragen),
  dus er staat na afloop ook niets in de ketel dat teruggezet moet worden.

## Wat de twee fasen doen
Twee instellingen in EMS-ESP klinken hetzelfde maar zijn het niet (gecontroleerd in de broncode van v3.9.1):

| Instelling | Wat er nog de bus op gaat | Wat het tegenhoudt |
| --- | --- | --- |
| **EMS Tx Mode = Off** | Niets. EMS-ESP antwoordt niet eens op de polls van de ketel (`TxService::send_poll`). Puur meeluisteren | Alles: geen leesvragen, geen schrijfopdrachten, geen versievraag |
| **Enable read-only mode** (Tx aan) | Poll-antwoorden en **leesvragen** (versie, waarden) van EMS-ESP zelf | Schrijfopdrachten (`TX_WRITE`): EMS-ESP logt `[readonly] Sending write Tx telegram: ...` en stuurt niets. **Let op:** ruwe `send`-telegrammen (console `send`, *Send telegram* in de web-UI) gaan in v3.9.1 wél door; dat is pas in v3.10.0 dicht. Gebruik ze niet |

- **Fase 1, Tx Off, ≥ 24 u:** dit is de log voor TQ-09. Je hoort hoe de Nefit-thermostaat en de ketel met elkaar
  praten, zonder dat EMS-ESP iets toevoegt. EMS-ESP herkent de apparaten toch: de thermostaat vraagt zelf versie en
  parameters van de ketel op, en EMS-ESP luistert die antwoorden mee (gezien in de test van 2026-11-25). Dat EMS-ESP
  zelf niets vroeg, laat `show ems` zien: `#read requests sent: 0` en `#write requests sent: 0`.
- **Fase 2, read-only met Tx aan, ≥ 1 u:** EMS-ESP stelt leesvragen, herkent ketel en thermostaat met naam en
  versie en toont hun waarden. Dat is de opstap naar TQ-10. Schrijven blijft geblokkeerd.

Adressen op de bus (eerste byte van een telegram = afzender): ketel `08`; Nefit-thermostaat `17` (Moduline 100/200/300,
RC20), `10` (Moduline 400, RC30/35) of `18` (Moduline 1000/Easy); EMS-ESP zelf standaard `49` (*Gateway 2*, de
standaard sinds v3.8.1). Een telegram met tweede byte ≥ `80` is een **leesvraag**; tweede byte `< 80` is een
**schrijfopdracht** of (bij `00`) een broadcast. Hoe je daarmee bewijst dat EMS-ESP niets schreef: zie fase 1 en 2.

## Onderdelen
| Item | Winkel, art.nr. | Prijs (gezien) | Status |
| --- | --- | --- | --- |
| BBQKees EMS Interface Board V3.1 | bbqkees-electronics.nl, `ems-int-v3-1` | € 34,95 (2026-11-25) | binnen sinds 2026-11-25 |
| EMS service cable 50 cm (3,5 mm-jack, verlengde plug; BBQKees: "do not use other jack cables") | bbqkees-electronics.nl, `ems-service-cable-50cm-angled` | € 4,95 (2026-11-25, toen uitverkocht) | in huis? ☐ ja ☐ nee |
| Alternatief: EMS service cable 150 cm (110 cm is uit het assortiment) | bbqkees-electronics.nl | niet gezien | alleen als de 50 cm niet leverbaar is |
| Alleen zonder service-jack: EMS screw terminal cable 100 cm (2 × 0,5 mm²) | bbqkees-electronics.nl, `ems-screw-terminal-cable-100cm` | € 4,95 (2026-11-25) | zie *Aansluiten* |
| Klassieke ESP32-WROOM-devkit uit de la (4 MB of meer) | in huis | — | type: ________ |
| 4 dupont-draadjes female-female | in huis | — | |
| USB-netadapter 5 V + USB-kabel met data | in huis | — | |
| Mac in hetzelfde netwerk, met deze repo | in huis | — | |

## Bedrading: interfacebord aan de ESP32
Het V3.1-bord verwacht **geen** Wemos D1 mini-footprint: het heeft een 4-pins UART-header **H1** (VCC, TX, RX, GND;
H3 heeft dezelfde pinvolgorde). Elke ESP32-devkit past dus met vier draadjes (BBQKees-wiki, *Interface board V3.1* en
*How to connect the boards*, gezien 2026-11-25).

| Pin op H1 | ESP32-devkit | Waarom |
| --- | --- | --- |
| VCC | **3V3** (nooit 5V/VIN) | Het bord haalt geen stroom uit de bus; de ESP voedt het. De spanning op RX/TX volgt VCC: 5 V op VCC = 5 V op de GPIO's = kapotte ESP32 |
| TX | **GPIO 5** | EMS-ESP *Tx GPIO*; niet kruisen: TX bij TX, RX bij RX (BBQKees-wiki) |
| RX | **GPIO 23** | EMS-ESP *Rx GPIO* |
| GND | **GND** | |

- [ ] Bedraad met de USB-kabel eruit. Foto van de bedrading in de run gezet
- [ ] Niet GPIO 1/3 (RX0/TX0) gebruikt: die zitten aan de USB-serieelchip
- [ ] De *EMS power out*-header van het bord blijft leeg (jackspanning 8–16 V; direct op een devkit = kapot)
- [ ] Niets aan GPIO 18 (EMS-ESP-standaard voor een temperatuursensor; mag leeg blijven)

Bordprofiel in EMS-ESP: **NodeMCU 32S** (`NODEMCU`): Rx GPIO 23, Tx GPIO 5, LED GPIO 2, knop GPIO 0. Dat zijn precies
de pinnen hierboven en die zitten op elke WROOM-devkit (LED 2 is op veel devkits de blauwe led, knop 0 is BOOT).
*MH-ET Live D1 Mini* en *S32* hebben dezelfde pinnen; *Custom* is niet nodig.

## EMS-ESP flashen (ESP32 alleen aan de Mac, niet aan de ketel)
Vastgepinde versie **EMS-ESP v3.9.1** (stabiele release van 2026-11-11, `src/emsesp_version.h` = `3.9.1`). De
firmware komt uit de officiële GitHub-release. De `.bin` is een **app-image** (gecontroleerd: `esptool image-info`,
geen bootloader erin) en hoort op `0x10000`; bootloader, partitietabel en `boot_app0` staan niet in de release maar in
de officiële docs-repo `emsesp/docs` (`static/bin/s_4M/`, profiel `s_4M`), hier vastgepind op commit `8403f2a` (ADR-017).

- [ ] Bestanden ophalen in een eigen map buiten de repo:

```bash
mkdir -p ~/theriq-emsesp && cd ~/theriq-emsesp
curl -LO https://github.com/emsesp/EMS-ESP32/releases/download/v3.9.1/EMS-ESP-3_9_1-ESP32-4MB.bin
curl -LO https://github.com/emsesp/EMS-ESP32/releases/download/v3.9.1/EMS-ESP-3_9_1-ESP32-4MB.md5
D=https://raw.githubusercontent.com/emsesp/docs/8403f2a0357b3cf801da16d99abbad06f9da325f/static/bin
curl -L -o bootloader.bin $D/s_4M/bootloader.bin
curl -L -o partitions.bin $D/s_4M/partitions.bin
curl -L -o boot_app0.bin $D/boot_app0.bin
shasum -a 256 *.bin
```

- [ ] `shasum` geeft exact (gecontroleerd 2026-11-25):
      `4d944c92…67f2` EMS-ESP-3_9_1-ESP32-4MB.bin (en `md5 -q` = inhoud van het `.md5`-bestand: `6f0a3c02cf14450ad9e4132e3167e976`),
      `f94c5d78…17f0` boot_app0.bin, `1370ee15…8f1d` bootloader.bin, `bee384e6…63e8` partitions.bin
- [ ] Poort gevonden: `ls /dev/cu.*` met en zonder USB; een klassieke devkit heeft een CP2102 of CH340 en heet
      `/dev/cu.usbserial-…` of `/dev/cu.wchusbserial…`. Poort: ________
- [ ] Chip en flash: `uvx --from esptool==5.1.0 esptool --port <poort> flash-id` toont `ESP32` en
      `Detected flash size: 4MB` (of groter). Gezien: ________
- [ ] Wissen (het bordje uit de la heeft oude inhoud; hier staan geen Matter-fabrics op):
      `uvx --from esptool==5.1.0 esptool --chip esp32 --port <poort> erase-flash`
- [ ] Flashen, eindigt zonder fout:

```bash
uvx --from esptool==5.1.0 esptool --chip esp32 --port <poort> --baud 460800 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 40m --flash-size detect 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 EMS-ESP-3_9_1-ESP32-4MB.bin
```

- [ ] (optioneel) Opstartlog: `screen <poort> 115200`, geen reboot-lus; afsluiten met Ctrl-A, K

## Eerste configuratie (nog steeds niet aan de ketel)
- [ ] Wifi van de Mac of telefoon naar het accesspoint `ems-esp`, wachtwoord `ems-esp-neo`; browser naar
      `http://192.168.4.1`, inloggen met `admin` / `admin` (fabriekswaarden v3.9.1)
- [ ] Netwerkinstellingen: eigen 2,4 GHz-netwerk, hostname `ems-esp`. Na opslaan valt het AP weg en zit EMS-ESP in
      het eigen netwerk. IP-adres (router of `ping ems-esp.local`): ________. Liefst een vaste DHCP-reservering
- [ ] Beveiligingsinstellingen: admin-wachtwoord en Super User-wachtwoord gewijzigd (niet in de repo zetten)
- [ ] NTP blijft uit en MQTT blijft uit (beide de fabrieksstand): geen verkeer naar internet, en Home Assistant krijgt
      geen entiteiten waarmee het iets naar de ketel zou kunnen sturen
- [ ] *Application Settings* → *Special Functions*: **Developer Mode** aan (nodig om Tx *Off* te kunnen kiezen)
- [ ] *Application Settings* → *Hardware Settings*: **Board Profile** = `NodeMCU 32S`; Rx GPIO 23 en Tx GPIO 5 zichtbaar
- [ ] *Hardware Settings*: **EMS Tx Mode** = `Off`
- [ ] *Hardware Settings*: **Enable read-only mode** aan (blijft in beide fasen aan, als tweede slot)
- [ ] *Services* → *Syslog*: **Enable Syslog** aan; Host = IP van de Mac (`ipconfig getifaddr en0`: ________);
      Port `5514`; Log Level `INFO`; Mark Interval `60`. INFO is nodig om ook een geblokkeerde schrijfpoging
      (`[readonly] ...`) in de log te zien; de telegrammen zelf komen op NOTICE
- [ ] IP van de Mac vast: wifi-*Details…* → **Privé-wifiadres** = Uit (een privé-adres wisselt en krijgt dan een
      ander IP; `ifconfig en0 | grep ether` toont dan een adres met `2`, `6`, `A` of `E` als tweede teken), en in
      de router een vaste koppeling voor de Mac en voor `ems-esp`. Andere bekende wifinetwerken: *Verbind
      automatisch* uit, anders springt de Mac bij een routerherstart naar een ander netwerk en loopt de log leeg
- [ ] Telnet staat aan (*Services*, standaard aan in v3.9.1). Opslaan; herstart EMS-ESP als de UI daarom vraagt
- [ ] *Modules* → *Commands* → **Add**: Command `system/watch`, Value `raw`, Name `watchraw`; **Apply changes**.
      Dan *Modules* → *Scheduler* → **Add**: knop **TIMER**, **Active** aan, tijd **00:00** (*at start-up*), Command
      `watchraw`, Name `watchstartup`; **Apply changes**. Zonder deze regel staat `watch` na elke herstart uit en stopt
      de log ongemerkt. Controle: EMS-ESP herstarten; in de syslog verschijnt `Called command system/watch … with value raw`
      (in read-only mode met `[readonly] ` ervoor: dat is een gewoon commando, geen buswrite)
- [ ] Controle: `nc ems-esp.local 23` (macOS heeft geen `telnet` meer), dan `show ems`: `Tx mode: 0`. Ctrl-C
- [ ] ESP32 van de Mac af. Vanaf nu voeding via de **USB-netadapter**, niet via de Mac: het interfacebord is niet
      galvanisch gescheiden (ADR-013), dus geen verbinding tussen de ketelbus en de Mac

## Aansluiten op de ketel
Route: de **service-jack** (3,5 mm). Eén stekker, niets losdraaien, eruit = oude situatie. Volgens BBQKees hebben
alle Nefit-ketels de jack aan de voorkant, vaak achter een rubberen of plastic klepje bij het display; voor de
TopLine Compact HRC specifiek is dat niet bevestigd (wiki toont alleen foto's). Gebruik **óf** de jack **óf** de
schroefklem, nooit allebei (kortsluitgevaar op de bus).

- [ ] Uitgangssituatie genoteerd: thermostaatdisplay normaal, ketel zonder storingscode, huidige kamertemperatuur en
      setpoint: ________ / ________. Foto van de ketelvoorkant
- [ ] Service-jack gevonden. Plek: ________ (foto in de run). Geen jack? Zie hieronder
- [ ] EMS-ESP draait op de netadapter, *Tx Mode* = Off bevestigd (`show ems`)
- [ ] Jackkabel eerst in het interfacebord, dan in de ketel
- [ ] Binnen een minuut: thermostaatdisplay normaal, geen storingscode. Anders: *Terugweg*

Geen service-jack: screw terminal cable parallel op de twee EMS-klemmen waar de thermostaatkabel in de ketel binnenkomt
(of op de muurplaat van de thermostaat). Eerst foto, ketel uit met de hoofdschakelaar, de bestaande draden blijven
zitten, de nieuwe draden komen erbij in dezelfde klemmen; ketel weer aan. Polariteit maakt meestal niet uit; zie je in
fase 1 geen telegrammen of wordt het thermostaatdisplay leeg, dan de twee nieuwe draden omwisselen (na *Terugweg*).

## Fase 1: Tx Off, meeluisteren, ≥ 24 u (de TQ-09-log)
De log loopt via syslog naar de Mac (ADR-017): EMS-ESP stuurt elke logregel als UDP-pakket naar poort 5514,
`tools/ems_syslog_capture.py` zet er de tijd van de Mac voor en schrijft hem naar een bestand.

- [ ] Mac aan de lader, slaapstand tegengehouden met `caffeinate` (klep dicht zonder extern scherm = toch slaap)
- [ ] Vanuit de repo-root, in een eigen terminaltab die 24 u open blijft:

```bash
caffeinate -i python3 tools/ems_syslog_capture.py docs/ketels/logs/<datum>-txoff.log
```

- [ ] Vraagt de macOS-firewall om inkomende verbindingen voor `python3` toe te staan: toestaan
- [ ] `watch raw` staat aan via de Scheduler-regel. Handmatig kan ook: `nc ems-esp.local 23`, dan `watch raw`
      (antwoord `Watching incoming telegrams, displayed as raw bytes`), Ctrl-C. `watch` is een instelling van EMS-ESP,
      niet van de telnetsessie, en verdwijnt alleen bij een herstart
- [ ] `tail -f docs/ketels/logs/<datum>-txoff.log`: regels als
      `2026-11-25T20:31:08.937<TAB><117>1 2026-11-25T20:32:11.000000+01:00 ems-esp telegram - - - 000+00:02:27.342 N 21: Rx: 08 00 34 00 …`.
      Velden: tijd van de Mac, syslog-kop met de tijd van EMS-ESP (overgenomen uit het tijdtelegram `06` van de
      thermostaat; die klok liep op 2026-11-25 ruim een minuut vóór), uptime van EMS-ESP (dagen+uu:mm:ss.mmm),
      niveau (`N` = NOTICE), volgnummer van de logregel, en de ruwe bytes inclusief CRC. Polls (één byte) staan er niet in.
      Regels `W … Tx queue overflow, skip one message` zijn verwacht: EMS-ESP zet leesvragen klaar die met Tx Off nooit
      verstuurd worden, en gooit bij een volle wachtrij de oudste weg
- [ ] **Bus gezien**: `grep -c 'Rx:' <log>` > 0 na een minuut: ________
- [ ] **Ketel gezien** (afzender `08`): `grep -c 'Rx: 08 ' <log>` > 0: ________
- [ ] **Thermostaat gezien**: `grep -cE 'Rx: (10|17|18) ' <log>`: ________; adres van de thermostaat: ________
      (type Nefit-thermostaat aan de muur: ________)
- [ ] Na 1 uur: thermostaat en ketel gedragen zich normaal, kamer op setpoint
- [ ] Na ≥ 24 u: capture stoppen met Ctrl-C. Eerste en laatste regel (`head -1`, `tail -1`) liggen ≥ 24 u uit elkaar
      en de laatste regel is een `Rx:`-regel (anders is EMS-ESP onderweg herstart en stond `watch` uit). Van ________ tot ________
- [ ] **Geen enkel telegram van EMS-ESP**: `grep -c 'Rx: 49 ' <log>` = 0 (met Tx Off zendt EMS-ESP niets), en
      `show ems` vóór het stoppen: `#read requests sent: 0`, `#write requests sent: 0`
- [ ] **Geen geblokkeerde schrijfpoging**: `grep -c '\[readonly\] Sending' <log>` = 0
- [ ] Verloren regels (UDP is best-effort; het volgnummer na het niveau loopt per logregel op, een sprong is verlies):

```bash
awk 'match($0, / [A-Z] [0-9]+: /) { n = substr($0, RSTART+3, RLENGTH-5) + 0; if (p && n > p+1) gaps += n-p-1; p = n } END { print "verloren berichten:", gaps+0 }' docs/ketels/logs/<datum>-txoff.log
```

- [ ] Verloren berichten: ________ (een handvol is geen probleem voor TQ-10; honderden: wifi-bereik bij de ketel
      nakijken en opnieuw)
- [ ] Grootte: ________ MB, ________ regels (`wc -l`). Verwachting: ruwweg 5–15 MB per etmaal (schatting, nog niet
      gemeten)

## Fase 2: read-only met leesvragen, ≥ 1 u
- [ ] Nieuwe capture: `caffeinate -i python3 tools/ems_syslog_capture.py docs/ketels/logs/<datum>-readonly.log`
- [ ] Web-UI: *EMS Tx Mode* = `Auto` (de fabrieksstand); **Enable read-only mode blijft aan**. Opslaan
- [ ] `nc ems-esp.local 23`: `show ems` toont `Tx mode: 5`, `EMS Bus is connected.` en `EMS Bus ID: 49`; dan opnieuw
      `watch raw`
- [ ] Nooit `send`, `call … <waarde>` of het wijzigen van een waarde in de web-UI: v3.9.1 laat ruwe `send` door
- [ ] **Ketel herkend**: web-UI *Devices* toont de ketel (adres 0x08). Naam, product-id, versie: ________
- [ ] **Thermostaat herkend**: naam, adres, product-id, versie: ________
- [ ] Apparaten en waarden bewaard voor TQ-10:
      `printf 'show devices\nshow values\n' | nc -w 5 ems-esp.local 23 > docs/ketels/logs/<datum>-values.txt`
- [ ] Na ≥ 1 u capture gestopt. **EMS-ESP stelde leesvragen**: `grep -cE 'Rx: 49 [89A-F][0-9A-F] ' <log>` > 0: ________
- [ ] **Geen schrijfopdracht van EMS-ESP**: `grep -cE 'Rx: 49 [0-7][0-9A-F] ' <log>` = 0. Eigen telegrammen komen als
      echo in de log (`EMSESP::incoming_telegram`), dus een write van `49` zou hier zichtbaar zijn
- [ ] `grep -c '\[readonly\] Sending' <log>` = 0 (> 0 betekent: iets probeerde te schrijven en is tegengehouden; noteer de
      regels onder *Open punten*)
- [ ] Thermostaat en ketel nog normaal

## Afronden
- [ ] Bord blijft hangen voor TQ-10? Dan *EMS Tx Mode* terug op `Off`. Anders: *Terugweg* (stekker eruit)
- [ ] Logs inpakken: `gzip -9 docs/ketels/logs/<datum>-*.log`; grootte gezipt: ________ MB
- [ ] Logs, `-values.txt` en het runbestand op een branch gecommit; PR naar `main`

## Open punten
- 
