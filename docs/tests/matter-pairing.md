# Checklist: koppelen in Apple Home en HA, multi-admin, R1 en R3 (TQ-06, WP0.2)

Gate 3: alleen Aernout vinkt af. Kopieer naar `docs/tests/runs/<datum>-TQ-06.md` voor een run. Uitgangspunt: het bord
draait de ketelunit-firmware van `main` (zie `docs/tests/thermostat-flash.md`) en is gewist (BOOT 10 s), dus het
wacht op koppelen. Er wordt niets naar een ketel geschreven.

Wat er bij het koppelen gebeurt (commissioning):
1. De iPhone vindt het bord via **BLE** en zet met de koppelcode een versleutelde sessie op (PASE: de code is het
   gedeelde geheim, hij gaat zelf niet over de lucht).
2. Over die BLE-sessie krijgt het bord de **wifi-gegevens** en een **operationeel certificaat** van Apple Home. Samen
   met de root-sleutel van Apple Home heet dat een **fabric**: een beheerdomein met eigen sleutels.
3. Het bord gaat op wifi en wordt via **mDNS** (IPv6, link-local) gevonden; vanaf dan loopt alles via een CASE-sessie
   over IP. BLE staat daarna uit.
4. **Multi-admin**: Apple Home opent een koppelvenster met een nieuwe, tijdelijke code. HA koppelt daarmee rechtstreeks
   over IP (geen BLE, wifi is er al) en krijgt een **tweede fabric**. Beide controllers abonneren zich op de attributen;
   een wijziging uit de ene app komt als rapport bij de andere. Dat is R3.

## Benodigd
- [ ] Waveshare ESP32-C6-Zero met ketelunit-firmware, gewist. Commit van het image: ________
- [ ] USB-C-datakabel naar de Mac (seriële log en console)
- [ ] iPhone met Apple Home, Bluetooth aan, op het 2,4 GHz-wifi (de C6 kent geen 5 GHz)
- [ ] Woningcentrale van Apple Home (HomePod of Apple TV) aan en bereikbaar
- [ ] Home Assistant met de Matter-integratie (zie hieronder)
- [ ] Stopwatch (telefoon) of een tweede persoon; of twee schermen naast elkaar en een video met de telefoon

## Home Assistant voorbereiden
- [ ] Instellingen → Apparaten en diensten → **Matter** toevoegen. Op HA OS installeert dat de add-on *Matter Server*;
      in Docker: een eigen `python-matter-server`-container met `--network host`. Versie Matter Server: ________
- [ ] Instellingen → Systeem → Netwerk: **IPv6** op de netwerkinterface staat aan (Automatisch). Matter gebruikt
      IPv6 voor mDNS en het verkeer met het apparaat
- [ ] HA zit in hetzelfde netwerk/VLAN als het bord; mDNS (multicast) wordt niet geblokkeerd
- [ ] Companion-app van HA op de iPhone (nodig voor "Matter-apparaat toevoegen": de app geeft de code door)

## QR-code lokaal maken
- [ ] Console: `matter onboardingcodes ble`. QR-payload: `MT:________`, handmatige code: ________
- [ ] PNG maken zonder webdienst:
      `uvx --from "qrcode[pil]==8.2" qr "MT:<payload>" > theriq-qr.png` en openen op de Mac
      (de `QRCodeUrl` uit de console stuurt de code naar een website: niet gebruiken)

## Koppelen in Apple Home
- [ ] Seriële log open laten lopen
- [ ] Woning-app → **+** → Voeg accessoire toe → QR-code scannen (of "Meer opties" en de handmatige code)
- [ ] Melding **"Niet-gecertificeerd accessoire"** verschijnt (verwacht, test-vendor-ID 0xFFF1, SPEC §3) → "Voeg toch toe"
- [ ] Log: `Commissioning session started`, dan `Fabric added, fabrics now: 1`, dan `Commissioning complete`
- [ ] Accessoire staat als thermostaat in de Woning-app, **niet** als "Niet ondersteund"
- [ ] Kamertemperatuur zichtbaar in Apple Home: ________ °C, gelijk aan de log (0x0000 / 100) binnen 0,1 °C
- [ ] Modus toont Verwarmen/Uit; geen Koelen of Auto

## Multi-admin: koppelen in HA
- [ ] Woning-app → accessoire → instellingen → **Schakel koppelmodus in** → code noteren: ________
      (de code geldt een paar minuten; log: koppelvenster open)
- [ ] HA-companion-app → Instellingen → Apparaten → Matter → Apparaat toevoegen → **"Nee, het is al in gebruik"**
      (of: "Het is al toegevoegd aan een andere app") → code invoeren
- [ ] Log: `Fabric added, fabrics now: 2`, `Commissioning complete`
- [ ] Apparaat in HA met een climate-entiteit; kamertemperatuur gelijk aan Apple Home: ________ °C
- [ ] Apple Home werkt nog na het koppelen in HA (setpoint even aanpassen en terugzetten)

## R3: synchroon binnen 2 s
Meet van loslaten in de ene app tot de nieuwe waarde zichtbaar is in de andere. Log toont
`OccupiedHeatingSetpoint -> xx.xx C` op het moment dat het bord de wijziging krijgt.
- [ ] Setpoint in Apple Home naar 21,5 °C → zichtbaar in HA na ________ s (< 2 s)
- [ ] Setpoint in HA naar 19,0 °C → zichtbaar in Apple Home na ________ s (< 2 s)
- [ ] Nog één keer beide kanten (spreiding): ________ s en ________ s
- [ ] Modus Uit in Apple Home → HA toont `off` na ________ s; log `SystemMode -> Off`
- [ ] Modus Verwarmen in HA → Apple Home toont Verwarmen na ________ s; log `SystemMode -> Heat`

## R1: zonder internet
- [ ] **WAN-kabel uit de router** (of het modem uit). Controle: op de Mac geeft `ping 1.1.1.1` geen antwoord.
      Tijdstip: ________
- [ ] 1 minuut wachten (lopende cloudsessies laten verlopen)
- [ ] Setpoint in Apple Home naar 20,5 °C → HA na ________ s
- [ ] Setpoint in HA naar 21,0 °C → Apple Home na ________ s
- [ ] Kamertemperatuur blijft verversen in beide apps (log elke 10 s)
- [ ] **WAN-kabel terug**; internet werkt weer (`ping 1.1.1.1`), beide apps blijven het bord tonen

## Open punten
- 
