# TherIQ — Stand

De overdracht, één scherm: wat nu waar is, wat de volgende stap is, wat op wie wacht. Geen geschiedenis: wat gebouwd
is staat in `BACKLOG.md`, waarom in `DECISIONS.md`. Herschrijven, nooit aanvullen. Maximaal 70 regels.

## Nu
- Fase 0, bewijs op devkits. Mijlpaal 31-12-2026: TherIQ regelt de Nefit via EMS, kamerunit-prototype aan de wand
  (ADR-008).
- De ketelunit draait op de Waveshare C6-Zero als Matter-thermostaat (endpoint 1, Off/Heat) met een nepsensor, en zit
  tegelijk in Apple Home en Home Assistant (multi-admin, R3 gehaald). R1 wordt gemeten in de run van TQ-29 (ADR-015).
- Host tests draaien in CI als job `host-tests` (CMake + CTest + `assert`, ADR-016). Pure logica staat in
  componenten onder `firmware/ketelunit/components/`, zonder IDF-includes; `firmware/host_tests/` test dezelfde bron.
- De EMS-framelaag staat er: component `ems_frame` (CRC, decode, encode; EMS 1.0 en EMS+), getest met echte
  telegrammen uit EMS-ESP v3.9.1. De firmware gebruikt hem nog niet; dat doet de EMS-driver (TQ-14).
- HA draait op HA OS op een Raspberry Pi 4 (192.168.50.39) met de Matter-integratie.
- OpenTherm is geparkeerd tot nader order (ADR-019): eerst werkt TherIQ voor dit huis, via EMS. TQ-16 t/m TQ-21
  staan onder *Geparkeerd* in de backlog; de gate van fase 0 vraagt geen OpenTherm meer.
- Het BBQKees EMS Interface Board V3.1 is binnen (2026-11-25). De checklist voor TQ-09 staat klaar:
  `docs/tests/ems-esp-readonly.md` (EMS-ESP v3.9.1 op de klassieke ESP32, eerst ≥ 24 u met Tx Off, dan read-only;
  log via syslog naar de Mac met `tools/ems_syslog_capture.py`, ADR-017).

## Volgende
Aan de ketel: Aernouts run van TQ-09 (zie *Wacht op Aernout*). Met die log: TQ-10 (ketelbeschrijving), daarna TQ-14
(EMS-driver op de C6 met hetzelfde bord).

Werk zonder hardware, elk als eigen build of samen voorgesteld op de `Builds:`-regel:
1. TQ-11: ketel-API (`boiler_api`) als C++-interface met ADR. Daarna weet TQ-14 waar hij heen schrijft.
2. TQ-22: aan/uit-regelaar met hysterese als pure logica met host tests.
3. TQ-23: weekschema (nachtverlaging) als pure logica met host tests.

Zodra de SHT45-breakout er is: TQ-07 (SHT45-driver) + TQ-31 (vendor- en productnaam) als één build die toch flasht.

## Wacht op Aernout
- Run TQ-09 (`docs/tests/ems-esp-readonly.md`): proef op 2026-11-25 geslaagd (`docs/tests/runs/2026-11-25-TQ-09-test.md`).
  De 24 u-log (fase 1, Tx Off) loopt van za 2026-11-28 06:00 tot zo 06:00. Vooraf: herstarttest van de
  Scheduler-regel `watchstartup` en het IP van de Mac (192.168.50.75) vastzetten.
- Inkoop (`docs/FASE-0.md`, inkooplijst): bij HobbyElectronica de ESP32-C6 DevKit C 16 MB (HE1251), de rotary
  encoder, de 12 V-voeding en de step-down; bij BerryBase de SHT45 en het 2,9"-e-paper (WS-19408); bij TinyTronics de
  logic analyzer. E-paper en encoder eerst: zonder die twee kan TQ-27 niet beginnen.

## Geblokkeerd
- TQ-10 en TQ-14: wachten op de buslog van TQ-09. TQ-07: wacht op de SHT45-breakout.
- Gate van fase 0: R1 wacht op TQ-29 (kamerunit aan de wand).

## Aan Aernout te beslissen
- IDEA-01 (koppelvenster opnieuw openen zonder reset) triëren.

## Goed om te weten
- Host tests lokaal (alleen `cmake` en een compiler, geen Docker), vanuit de repo-root:
  `cmake -S firmware/host_tests -B build/host && cmake --build build/host && ctest --test-dir build/host --output-on-failure`
  Een nieuwe test: testbestand in `firmware/host_tests/`, `add_executable` + `add_test(NAME TQ-nn_...)` erbij.
- Geen Docker op de Mac en weinig schijfruimte: de firmware bouwt in GitHub Actions. Flashen met de ESP Web Flasher of
  `uvx --from esptool==5.1.0 esptool --port /dev/cu.usbmodem1101 write-flash 0x0 <bin>`.
  Flashen **zonder** `erase-flash` behoudt de fabrics; met erase moet opnieuw gekoppeld worden.
- Zonder fabric adverteert het bord maar ~15 min via BLE na het opstarten; daarna eerst resetten, dan koppelen.
- BOOT 10 s = Matter-fabrieksreset met led-aftelling (ADR-014). Bord niet resetten zonder reden.
- Een extra controller koppelen: Apple Home → accessoire → Schakel koppelmodus in; die code in de andere app.
- QR-code lokaal: `uvx --from "qrcode[pil]==8.2" qr "MT:..." > qr.png`; niet de `QRCodeUrl` uit de log (website).
- De huidige Nefit-thermostaat blijft op de EMS-bus als terugvaloptie tot WP0.8. Daarna voedt de thermostaatkabel (2 aders)
  het kamerunit-prototype (ADR-018).
- EMS-ESP v3.9.1: *read-only mode* blokkeert schrijfopdrachten maar niet ruwe `send`-telegrammen (pas dicht in
  v3.10.0); *EMS Tx Mode* Off zendt helemaal niets. Het BBQKees-bord is niet galvanisch gescheiden: aan de ketel de
  ESP via een netadapter voeden, niet via de Mac.
- Waveshare C6-Zero: flashen lukt niet? BOOT ingedrukt houden bij het insteken van USB. Led wil RGB-volgorde
  (`CONFIG_THERIQ_RGB_LED_ORDER_RGB=y`); op de DevKitC-1 waarschijnlijk GRB.
- Seriële console via USB: `matter onboardingcodes ble`, `matter config`, `matter esp attribute get 0x1 0x201 0x1c`.
- EMS-type-id's in TherIQ volgen EMS-ESP: EMS+-typebytes `01 A5` zijn type `0x02A5`. Neem id's uit EMS-ESP letterlijk.
