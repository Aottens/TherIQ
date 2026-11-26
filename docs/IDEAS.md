# TherIQ — Ideeën

Aernout gooit ideeën in de chat, in elke vorm. Claude zet ze hier, met een id `IDEA-nn`, en bewaakt de scope. Een
idee is **geen** toezegging en krijgt geen taak of code tot zijn fase gepland is.

Hoe een idee beweegt: **nieuw** → **getrieerd** (Claude stelt een fase voor, wat het nu raakt, open vragen; Aernout
bevestigt) → **backlog** (een TQ-rij) → **klaar** of **afgewezen** (met reden, zodat het niet opnieuw opduikt).

Scope-regel: v1 is `docs/SPEC.md` met de mijlpaal van ADR-008. Een nieuw idee landt in een latere fase, tenzij Aernout
het expliciet naar voren haalt. Dwingt een idee nu al een beslissing af, dan komt alleen die beslissing in de backlog.

| Id | Idee | Status | Fase |
| --- | --- | --- | --- |
| IDEA-01 | Koppelvenster opnieuw openen zonder reset: een korte druk op BOOT (of later de draaiknop of web-UI) start BLE-adverteren opnieuw als het bord nog geen fabric heeft. Nu sluit het venster ~15 min na opstarten (deelrun TQ-06, 2026-11-19). | nieuw | 0 of 2 |
| IDEA-02 | Weekschema in NVS bewaren en bewerken via de web-UI en HA (SPEC §7, *Modi en schema*). Nu staat er alleen een standaardschema in code (ADR-021, TQ-23). | nieuw | 1 |
| IDEA-03 | Vorstbeveiliging van het huis in modus Uit (SPEC §7, *Modi*): onder bv. 6 °C toch stoken. De regelaar van TQ-22 is in Uit nu altijd uit (ADR-021). | nieuw | 0 of 1 |
