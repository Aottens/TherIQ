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

## Volgende
Werk zonder hardware, elk als eigen build of samen voorgesteld op de `Builds:`-regel:
1. TQ-11: ketel-API (`boiler_api`) als C++-interface met ADR. Daarna weet TQ-14 waar hij heen schrijft.
2. TQ-16: OpenTherm-frame encode/decode en pariteit met host tests (zelfde patroon als `ems_frame`).
3. TQ-22: aan/uit-regelaar met hysterese als pure logica met host tests.

Zodra de SHT45-breakout er is: TQ-07 (SHT45-driver) + TQ-31 (vendor- en productnaam) als één build die toch flasht.
Zodra het BBQKees-bord er is: TQ-09 (alleen lezend aan de ketel, 24 u buslog), dan TQ-10 en TQ-14.

## Wacht op Aernout
- Inkoop (`docs/FASE-0.md`, inkooplijst). Eerst het **BBQKees EMS Interface Board V3.1** (bbqkees-electronics.nl,
  € 34,95): kritieke pad naar de mijlpaal van 31-12 (WP0.4 en WP0.5, ADR-013). Verder: SHT45-breakout, logic
  analyzer, 2× ESP32-C6-DevKitC-1-N8, USB-C-datakabel.
- PR #6 (Build B) reviewen en mergen.

## Geblokkeerd
- TQ-07, TQ-09 en alles daarna op de ketel: wacht op de inkoop.
- Gate van fase 0: R1 wacht op TQ-29 (kamerunit aan de wand).

## Aan Aernout te beslissen
- WP0.9: hoe het kamerunit-prototype aan de wand voeding en verbinding krijgt (TQ-26).
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
- De huidige Nefit-thermostaat blijft op de EMS-bus als terugvaloptie tot WP0.8.
- Waveshare C6-Zero: flashen lukt niet? BOOT ingedrukt houden bij het insteken van USB. Led wil RGB-volgorde
  (`CONFIG_THERIQ_RGB_LED_ORDER_RGB=y`); op de DevKitC-1 waarschijnlijk GRB.
- Seriële console via USB: `matter onboardingcodes ble`, `matter config`, `matter esp attribute get 0x1 0x201 0x1c`.
- EMS-type-id's in TherIQ volgen EMS-ESP: EMS+-typebytes `01 A5` zijn type `0x02A5`. Neem id's uit EMS-ESP letterlijk.
