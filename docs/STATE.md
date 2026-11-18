# TherIQ — Stand

De overdracht, één scherm: wat nu waar is, wat de volgende stap is, wat op wie wacht. Geen geschiedenis: wat gebouwd
is staat in `BACKLOG.md`, waarom in `DECISIONS.md`. Herschrijven, nooit aanvullen. Maximaal 70 regels.

## Nu
- Fase 0, bewijs op devkits. Mijlpaal 31-12-2026: TherIQ regelt de Nefit via EMS, kamerunit-prototype aan de wand
  (ADR-008).
- De CI bouwt de lege Matter-app groen; de repo is publiek op github.com/Aottens/TherIQ.

## Volgende
1. TQ-02: de CI-build flashen op de Waveshare C6-Zero (Aernout, thuis).
2. Build A: TQ-04 + TQ-05, de Matter-thermostaat met nepsensor en fabrieksreset (WP0.2).

## Wacht op Aernout
Runs die alleen Aernout kan doen (gate 3):
- TQ-02 flashen en opstartlog (`docs/tests/devkit-flash.md`).

Inkoop (`docs/FASE-0.md`, inkooplijst):
- 2× ESP32-C6-DevKitC-1-N8, SHT45-breakout, logic analyzer, USB-C-datakabel.
- EMS-ESP-gateway (BBQKees) voor WP0.4: vaak lange levertijd, vroeg bestellen.

## Geblokkeerd
- Niets.

## Aan Aernout te beslissen
- WP0.9: hoe het kamerunit-prototype aan de wand voeding en verbinding krijgt (TQ-26).

## Goed om te weten
- Geen Docker op de Mac en weinig schijfruimte: bouwen gebeurt in GitHub Actions; flashen met de ESP Web Flasher.
- De huidige Nefit-thermostaat blijft op de EMS-bus als terugvaloptie tot WP0.8.
- Waveshare C6-Zero: flashen lukt niet? BOOT ingedrukt houden bij het insteken van USB.
