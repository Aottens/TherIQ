# Beslissingen (ADR-log)

Eén regel per beslissing hieronder, met datum, status en reden. Nieuwe beslissingen onderaan toevoegen; een herziene beslissing krijgt een nieuw nummer dat naar de oude verwijst.

| ADR | Datum | Besluit | Status | Reden |
| --- | --- | --- | --- | --- |
| 001 | 2026-11-18 | Matter over wifi als primaire interface, niet HomeSpan | Voorstel (D1) | Alleen Matter multi-admin geeft tegelijk Apple Home én HA (R3) |
| 002 | 2026-11-18 | Twee units: ketelunit (alle intelligentie) + kamerunit (meten, tonen, bedienen) over de bestaande thermostaatkabel | Aangenomen | OT-busvermogen te laag voor wifi; geen stopcontact bij de thermostaatplek; R6 |
| 003 | 2026-11-18 | Ketelunit spreekt OpenTherm én EMS achter één abstracte ketel-API; aan/uit als fallback | Aangenomen | Eigen ketel (Nefit TopLine Compact HRC 25/CW4) is EMS; Bosch-familie groot in NL |
| 004 | 2026-11-18 | ESP32-C6-WROOM-1-N8 als MCU ketelunit | Aangenomen | Wifi 6 + Thread + BLE, voorgecertificeerd, ruimte voor Matter + OTA |
| 005 | 2026-11-18 | Licenties: firmware GPL-3.0-or-later, hardware CERN-OHL-S-2.0, docs CC BY-SA 4.0 | Aangenomen (WP0.1) | Copyleft houdt het ontwerp open; compatibel met EMS-ESP (GPL-3.0) |
| 006 | 2026-11-18 | Fase 0 met kant-en-klare EMS-ESP-gateway om de ketel te leren kennen; eigen EMS-interface daarna | Aangenomen (D8) | Eerst data, dan ontwerp |
| 007 | 2026-11-18 | Projectnaam: TherIQ | Aangenomen (D6) | Bestaande naam van Aernout; geen commercieel doel, wel open en zo betaalbaar mogelijk |
| 008 | 2026-11-18 | Mijlpaal 31-12-2026: TherIQ werkend op devkits (ketelunit regelt Nefit via EMS, kamerunit-prototype aan de wand); eigen PCB's daarna | Aangenomen | Volledige v1 met twee eigen printen past niet in 6 weken; het echte probleem (verwarming 's nachts aan) is hiermee wel opgelost |
| 009 | 2026-11-18 | Kosteneis R10: materiaal ≤ € 100 per set | Aangenomen | 'Zo betaalbaar als mogelijk' meetbaar maken; stuurt keuzes in display, MCU en kamerbus |
| 010 | 2026-11-18 | Toolchain vastgepind: Docker-image `espressif/esp-matter:release-v1.6.1_idf_v6.0.2` op digest `sha256:a55d497c…c4e6`; gebruikt door devcontainer en CI | Aangenomen | Nieuwste esp-matter-releasebranch met stabiele IDF 6.0.x; digest i.p.v. tag omdat Espressif tags opnieuw bouwt. Bijwerken alleen via nieuwe ADR |
| 011 | 2026-11-18 | Werkwijze uit CMDMX overgenomen: STATE-overdracht, backlog met TQ-taken, builds van ≤ 3 taken met een subagent per taak, gate-3-checklists, ideeën-inbox. Niet overgenomen: GitHub-review, risk gate, automerge, tracking-CI, dashboard | Aangenomen | Context per sessie en subagent klein houden; review doet Aernout zelf bij de merge. Tracking-check in CI pas als STATE en BACKLOG gaan afdrijven |
| 012 | 2026-11-18 | Prototypebehuizing (WP0.9) in Fusion; STEP en STL naast het Fusion-bestand in de repo | Aangenomen (Aernout) | Aernouts eigen gereedschap, snelste weg naar de mijlpaal. Wijkt af van SPEC §9 (alleen open CAD): het Fusion-bronbestand is alleen in Fusion parametrisch te bewerken. Voor de v1-behuizing (fase 2) opnieuw beslissen vóór publicatie (R8) |
| 013 | 2026-11-18 | EMS in fase 0: BBQKees EMS Interface Board V3.1 in plaats van de complete gateway. WP0.4 met EMS-ESP (release `ESP32-4MB`) op een klassieke ESP32-WROOM uit de la; WP0.5 met hetzelfde bord aan de C6. Herziet ADR-006 en beantwoordt D8 | Aangenomen | € 35 in plaats van € 120 en één bord voor beide werkpakketten; EMS-ESP levert een officiële 4 MB-build voor de klassieke ESP32. Bord is niet galvanisch gescheiden (polyfuse + TVS); we kopen het, nemen het schema niet over (geen licentiecheck nodig, harde regel 5) |
