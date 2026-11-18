# Checklist: thermostaat-endpoint en fabrieksreset op de devkit (TQ-04, TQ-05, WP0.2)

Gate 3: alleen Aernout vinkt af. Kopieer naar `docs/tests/runs/<datum>-TQ-04-05.md` voor een run. Kan in één sessie met
TQ-02 (`docs/tests/devkit-flash.md`): flash dan dit image en vink daar de algemene punten af. Niet koppelen met Apple
Home of HA: dat is TQ-06.

## Benodigd
- [ ] Waveshare ESP32-C6-Zero (esptool: ESP32-C6FH4 rev v0.2, 8 MB flash; in huis) of ESP32-C6-DevKitC-1-N8
- [ ] USB-C-kabel die data doet (niet alleen laden)
- [ ] Chrome op de Mac

## Image ophalen
- [ ] Groene run van de PR-branch of van `main` na de merge: github.com/Aottens/TherIQ/actions → workflow *firmware*
      → artifact onderaan. Commit van de run: ________
- [ ] Zip uitgepakt; `build/theriq-ketelunit.bin` aanwezig

## Flashen
- [ ] espressif.github.io/esptool-js, **Connect**, poort `USB JTAG/serial debug unit`
      (lukt het niet: BOOT ingedrukt houden tijdens het insteken van USB, dan opnieuw Connect)
- [ ] **Erase Flash** eerst (oude fabrics en wifi weg), daarna Flash Address `0x0`, `theriq-ketelunit.bin`,
      **Program**: eindigt zonder fout
- [ ] USB eruit en weer erin (reset)

## Opstartlog (Console op dezelfde pagina, 115200 baud)
- [ ] Geen reboot-lus (geen herhaalde `rst:` of `Guru Meditation`)
- [ ] Regel `Thermostat on endpoint 1, LocalTemperature 2x.xx C (fake sensor)`. Gezien: ________
- [ ] Die regel staat **vóór** de regels van de Matter-stack (`SetupQRCode: [MT:...]`, `Manual pairing code`)
- [ ] Regel `TherIQ boiler unit started`
- [ ] Geen regel `RGB LED on GPIO8 not available`
- [ ] Console: `matter esp attribute get 0x1 0x201 0x0` toont een waarde tussen 1900 en 2100 (0,01 °C);
      na een minuut opnieuw: de waarde is veranderd. Gezien: ________ en ________
- [ ] Console: `matter esp attribute get 0x1 0x201 0x1c` toont `4` (SystemMode Heat)
- [ ] Console: `matter esp attribute get 0x1 0x201 0x1b` toont `2` (ControlSequenceOfOperation Heating only)

## Fabrieksreset (BOOT 10 s)
- [ ] BOOT 3 s ingedrukt en loslaten: led knippert blauw, log telt af (`Factory reset in 9 s`, ...), dan
      `Factory reset cancelled`; led uit, bord draait door
- [ ] BOOT 10 s ingedrukt: eerst blauw knipperen, de laatste 3 s rood knipperen, dan rood vast
- [ ] Log toont `Factory reset: erasing fabrics and Wi-Fi, then restart` en het bord start opnieuw op
- [ ] Na de herstart: opnieuw `SetupQRCode: [MT:...]` (wacht op koppelen)
- [ ] Rood en groen verwisseld (de laatste 3 s groen in plaats van rood)? Ja / nee: ________
      (ja: `CONFIG_THERIQ_RGB_LED_ORDER_RGB=y` in `firmware/ketelunit/sdkconfig.defaults`, als open punt)

## Open punten
- 
