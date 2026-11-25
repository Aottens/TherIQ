# Ruwe buslogs van de eigen ketel

Ruwe EMS-buslogs van de Nefit TopLine Compact HRC 25/CW4, opgenomen met EMS-ESP op een klassieke ESP32 met het
BBQKees EMS Interface Board V3.1, alleen lezend (TQ-09, checklist `docs/tests/ems-esp-readonly.md`). Bron voor TQ-10
(`docs/ketels/nefit-topline-compact-hrc25-cw4.md`) en testvectoren voor de EMS-driver (TQ-14).

## Naamgeving
- `<startdatum>-txoff.log.gz`: fase 1, EMS-ESP met *EMS Tx Mode* = Off, ≥ 24 u. EMS-ESP zendt niets; alleen ketel en
  thermostaat praten.
- `<startdatum>-readonly.log.gz`: fase 2, Tx aan met *read-only mode*; bevat ook de leesvragen van EMS-ESP (adres `49`).
- `<startdatum>-values.txt`: uitvoer van `show devices` en `show values` uit de EMS-ESP-console in fase 2.

Versie van EMS-ESP, bordprofiel en de afvinkpunten staan in het runbestand `docs/tests/runs/<datum>-TQ-09.md`.

## Formaat
Eén regel per syslog-bericht van EMS-ESP, geschreven door `tools/ems_syslog_capture.py` (ADR-017):

```
2026-11-27T10:00:01.234<TAB><13>1 - ems-esp emsesp - - - 000+00:12:34.567 N 41: Rx: 08 00 18 00 …
```

| Veld | Betekenis |
| --- | --- |
| `2026-11-27T10:00:01.234` | Lokale tijd van de Mac bij ontvangst (ms). EMS-ESP heeft zonder NTP geen klok |
| `<13>1 - ems-esp emsesp - - -` | Syslog-kop (RFC 5424); `-` = geen tijd van EMS-ESP |
| `000+00:12:34.567` | Uptime van EMS-ESP (dagen+uu:mm:ss.mmm): de nauwkeurige tijdbasis voor pollritme en antwoordtijden |
| `N` | Niveau: `N` = NOTICE (telegrammen), `I` = INFO, `W` = WARNING |
| `41` | Volgnummer van de logregel; een sprong betekent een verloren UDP-pakket |
| `Rx: 08 00 18 00 …` | Ruw telegram in hex, inclusief CRC: afzender, ontvanger (≥ `80` = leesvraag), type, offset, data, CRC |

Niet in de log: de één-byte-polls van de ketel en de één-byte-bevestigingen van schrijfopdrachten (EMS-ESP logt
alleen telegrammen van vijf bytes of meer). Telegrammen die EMS-ESP zelf verstuurt, komen als echo wél in de log.

## Grootte
Schatting, nog niet gemeten: 5–15 MB per etmaal ongecomprimeerd. Logs gaan gezipt (`gzip -9`) in de repo; lezen met
`gzcat <bestand> | less`. De werkelijke grootte staat in het runbestand.
