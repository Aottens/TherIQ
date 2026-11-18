# TherIQ

Open-source, modulerende thermostaat voor OpenTherm- en EMS-ketels. Volledig lokaal, geen cloud, geen abonnement, tegelijk te bedienen vanuit Apple Home en Home Assistant via Matter. Na te bouwen met printen van JLCPCB en een 3D-geprinte behuizing.

Status: fase 0, bewijs op devkits. Zie [`docs/SPEC.md`](docs/SPEC.md) en [`docs/FASE-0.md`](docs/FASE-0.md).

## Repo-indeling

| Map | Inhoud | Licentie |
| --- | --- | --- |
| `firmware/ketelunit` | ESP-IDF + esp-matter, ESP32-C6 | GPL-3.0-or-later |
| `firmware/kamerunit` | firmware kamerunit (volgt) | GPL-3.0-or-later |
| `firmware/ketelsim` | OpenTherm-ketelsimulator (volgt) | GPL-3.0-or-later |
| `tools` | ketelbus-logger, kalibratie (volgt) | GPL-3.0-or-later |
| `hardware/ketelunit`, `hardware/kamerunit` | KiCad-projecten (volgt) | CERN-OHL-S-2.0 |
| `enclosure` | parametrisch CAD-model + STEP/STL (volgt) | CERN-OHL-S-2.0 |
| `docs` | spec, beslissingen, testverslagen | CC BY-SA 4.0 |

De volledige licentieteksten staan in [`LICENSES/`](LICENSES). Bronbestanden dragen een `SPDX-License-Identifier`-regel.

## Firmware bouwen

De toolchain zit in één vastgepind Docker-image: esp-matter `release/v1.6.1` met ESP-IDF v6.0.2 (ADR-010). Lokaal bouwen kan via de devcontainer (`.devcontainer/`) of direct met Docker:

```bash
docker run --rm -v "$PWD":/project -w /project/firmware/ketelunit \
  espressif/esp-matter:release-v1.6.1_idf_v6.0.2@sha256:a55d497c9b53e5a65789360e47ffb5cc3622a386e7b5bb76d2129578dc33c4e6 \
  bash -c 'cd /project/firmware/ketelunit && idf.py build && idf.py merge-bin -o theriq-ketelunit.bin'
```

Zonder lokale toolchain: elke push bouwt in GitHub Actions; het image `theriq-ketelunit.bin` staat als artifact bij de run.

## Flashen

Het artifact is één samengevoegd image vanaf adres `0x0`.

- **Zonder installatie**: [ESP Web Flasher](https://espressif.github.io/esptool-js/) in Chrome, devkit via USB, bestand op offset `0x0`.
- **Met esptool**: `pipx run esptool --chip esp32c6 write-flash 0x0 theriq-ketelunit.bin`

Seriële log bekijken: `screen /dev/cu.usbmodem* 115200` (afsluiten met Ctrl-A, K) of de console van de Web Flasher.

## Beveiliging

Zie [`SECURITY.md`](SECURITY.md).
