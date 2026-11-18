# Checklist: CI-build flashen op een C6-devkit (TQ-02, WP0.1)

Gate 3: alleen Aernout vinkt af. Kopieer naar `docs/tests/runs/<datum>-TQ-02.md` voor een run.

## Benodigd
- [ ] Waveshare ESP32-C6-Zero (esptool: ESP32-C6FH4 rev v0.2, 8 MB flash; in huis) of ESP32-C6-DevKitC-1-N8
- [ ] USB-C-kabel die data doet (niet alleen laden)
- [ ] Chrome op de Mac

## Image ophalen
- [ ] Laatste groene run op `main`: github.com/Aottens/TherIQ/actions → workflow *firmware* → artifact onderaan
- [ ] Zip uitgepakt; `build/theriq-ketelunit.bin` aanwezig (~1,9 MB). Commit van de run: ________

## Flashen
- [ ] espressif.github.io/esptool-js geopend, **Connect**, poort `USB JTAG/serial debug unit` gekozen
      (lukt het niet: BOOT ingedrukt houden tijdens het insteken van USB, dan opnieuw Connect)
- [ ] Log van Connect toont `Chip is ESP32-C6` en flashgrootte 8MB. Gezien: ________
- [ ] Flash Address `0x0`, bestand `theriq-ketelunit.bin`, **Program**: eindigt zonder fout
- [ ] USB eruit en weer erin (reset)

## Opstartlog (Console op dezelfde pagina, 115200 baud)
- [ ] Geen reboot-lus (geen herhaalde `rst:` of `Guru Meditation`)
- [ ] Regel `TherIQ boiler unit started`
- [ ] Regels `SetupQRCode: [MT:...]` en `Manual pairing code: [...]`
- [ ] `matter onboardingcodes ble` in de console toont dezelfde codes
- [ ] Bordje na 5 minuten lauw, niet heet

## Open punten
- 
