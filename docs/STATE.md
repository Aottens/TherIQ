# TherIQ — Stand

De overdracht, één scherm: wat nu waar is, wat de volgende stap is, wat op wie wacht. Geen geschiedenis: wat gebouwd
is staat in `BACKLOG.md`, waarom in `DECISIONS.md`. Herschrijven, nooit aanvullen. Maximaal 70 regels.

## Nu
- Fase 0, bewijs op devkits. Mijlpaal 31-12-2026: TherIQ regelt de Nefit via EMS, kamerunit-prototype aan de wand
  (ADR-008).
- De ketelunit-firmware (`594135d`) draait op de Waveshare C6-Zero als Matter-thermostaat (endpoint 1, alleen
  verwarmen, Off/Heat) met een nepsensor (sinus 20 ± 1 °C, periode 10 min).
- Het bord zit tegelijk in **Apple Home en Home Assistant** (multi-admin, 3 fabrics: twee van Apple, één van HA).
  **R3 gehaald**: setpoint en modus komen in beide richtingen vrijwel direct over. Geen firmwarewijziging nodig.
- **R1 nog niet gemeten** (WAN-kabel uit): verschoven naar de run van TQ-29, met de kamerunit aan de wand (ADR-015).
- HA draait op HA OS op een Raspberry Pi 4 (192.168.50.39) met de Matter-integratie.
- BOOT 10 s = Matter-fabrieksreset met led-aftelling (ADR-014 voor pinnen en ledvolgorde).

## Volgende
1. TQ-03 (host tests in CI) als eerste bouwtaak; daarna TQ-07 (SHT45) zodra de breakout er is.
2. TQ-31 (vendor- en productnaam in Basic Information) is klein; kan mee in een build die toch flasht.

## Wacht op Aernout
- Geen open runs. R1 (WAN-kabel uit, mobiele data uit) komt in de checklist van TQ-29. Bord niet resetten.

Inkoop (`docs/FASE-0.md`, inkooplijst):
- 2× ESP32-C6-DevKitC-1-N8, SHT45-breakout, logic analyzer, USB-C-datakabel.
- BBQKees EMS Interface Board V3.1 (bbqkees-electronics.nl, € 34,95) voor WP0.4 en WP0.5 (ADR-013).

## Geblokkeerd
- Gate van fase 0: R1 wacht op TQ-29 (kamerunit aan de wand).

## Aan Aernout te beslissen
- WP0.9: hoe het kamerunit-prototype aan de wand voeding en verbinding krijgt (TQ-26).
- IDEA-01 (koppelvenster opnieuw openen zonder reset) triëren.

## Goed om te weten
- Geen Docker op de Mac en weinig schijfruimte: bouwen gebeurt in GitHub Actions. Flashen kan met de ESP Web Flasher
  of vanaf de Mac: `uvx --from esptool==5.1.0 esptool --port /dev/cu.usbmodem1101 write-flash 0x0 <bin>`.
  Een nieuw image flashen **zonder** `erase-flash` behoudt de fabrics; met erase moet opnieuw gekoppeld worden.
- Zonder fabric adverteert het bord maar ~15 min via BLE na het opstarten; daarna eerst resetten, dan koppelen.
- Een extra controller koppelen: Apple Home → accessoire → Schakel koppelmodus in; die code in de andere app.
- QR-code lokaal: `uvx --from "qrcode[pil]==8.2" qr "MT:..." > qr.png`; niet de `QRCodeUrl` uit de log (website).
- De huidige Nefit-thermostaat blijft op de EMS-bus als terugvaloptie tot WP0.8.
- Waveshare C6-Zero: flashen lukt niet? BOOT ingedrukt houden bij het insteken van USB.
- De led van de C6-Zero wil RGB-volgorde (`CONFIG_THERIQ_RGB_LED_ORDER_RGB=y`); op de DevKitC-1 waarschijnlijk GRB.
- Seriële console via USB: `matter onboardingcodes ble`, `matter config`, `matter esp attribute get 0x1 0x201 0x1c`.
  De kamertemperatuur (nullable) geeft daar een fout; die staat elke 10 s in de log.
