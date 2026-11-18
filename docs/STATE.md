# TherIQ — Stand

De overdracht, één scherm: wat nu waar is, wat de volgende stap is, wat op wie wacht. Geen geschiedenis: wat gebouwd
is staat in `BACKLOG.md`, waarom in `DECISIONS.md`. Herschrijven, nooit aanvullen. Maximaal 70 regels.

## Nu
- Fase 0, bewijs op devkits. Mijlpaal 31-12-2026: TherIQ regelt de Nefit via EMS, kamerunit-prototype aan de wand
  (ADR-008).
- De ketelunit-firmware is een Matter-thermostaat (endpoint 1, alleen verwarmen, Off/Heat) met een nepsensor
  (sinus 20 ± 1 °C, periode 10 min). De kamertemperatuur staat al vóór de start van de Matter-stack.
- BOOT 10 s ingedrukt = Matter-fabrieksreset, de RGB-led telt af (blauw, laatste 3 s rood). Board-pinnen zijn
  Kconfig-opties (ADR-014).
- Alles bouwt groen in CI; nog niets hiervan is op hardware gedraaid.

## Volgende
1. Aernout: TQ-02 en de thermostaat-run in één keer (`docs/tests/devkit-flash.md` en
   `docs/tests/thermostat-flash.md`) met het image van deze PR of van `main` na de merge.
2. TQ-06: koppelen in Apple Home, daarna multi-admin in HA (R1, R3). Pas na een geslaagde run van punt 1.
3. TQ-03 (host tests in CI) als eerste bouwtaak; daarna de SHT45 (TQ-07) in plaats van de nepsensor.

## Wacht op Aernout
Runs die alleen Aernout kan doen (gate 3):
- TQ-02 flashen en opstartlog (`docs/tests/devkit-flash.md`).
- TQ-04 + TQ-05 thermostaat en fabrieksreset (`docs/tests/thermostat-flash.md`).

Inkoop (`docs/FASE-0.md`, inkooplijst):
- 2× ESP32-C6-DevKitC-1-N8, SHT45-breakout, logic analyzer, USB-C-datakabel.
- BBQKees EMS Interface Board V3.1 (bbqkees-electronics.nl, € 34,95) voor WP0.4 en WP0.5 (ADR-013).

## Geblokkeerd
- Niets.

## Aan Aernout te beslissen
- WP0.9: hoe het kamerunit-prototype aan de wand voeding en verbinding krijgt (TQ-26).

## Goed om te weten
- Geen Docker op de Mac en weinig schijfruimte: bouwen gebeurt in GitHub Actions; flashen met de ESP Web Flasher.
- De huidige Nefit-thermostaat blijft op de EMS-bus als terugvaloptie tot WP0.8.
- Waveshare C6-Zero: flashen lukt niet? BOOT ingedrukt houden bij het insteken van USB.
- De led van de C6-Zero wil RGB-volgorde: `CONFIG_THERIQ_RGB_LED_ORDER_RGB=y` in `sdkconfig.defaults`; op de DevKitC-1
  waarschijnlijk `n` (GRB).
- Seriële console via USB: `matter onboardingcodes ble`, `matter esp attribute get 0x1 0x201 0x1c` (SystemMode).
  De kamertemperatuur (nullable) geeft daar een fout; die staat elke 10 s in de log.
