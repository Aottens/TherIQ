# Beveiligingsbeleid

TherIQ stuurt een cv-ketel aan en hangt in je thuisnetwerk. Kwetsbaarheden nemen we serieus.

## Een kwetsbaarheid melden

Meld kwetsbaarheden **niet** via een publieke issue. Gebruik **GitHub Private Vulnerability Reporting**: tabblad *Security* → *Report a vulnerability* in deze repo.

Vermeld:

- welke component (ketelunit-firmware, kamerunit, web-UI, Matter, hardware);
- firmwareversie of commit;
- stappen om het te reproduceren en de mogelijke impact.

## Wat je mag verwachten

- Ontvangstbevestiging binnen 7 dagen.
- Een eerste beoordeling binnen 30 dagen.
- Na een fix: vermelding in de release notes, met je naam als je dat wilt.

TherIQ is een hobbyproject zonder betaalde ondersteuning; dit zijn inspanningsdoelen, geen garanties.

## Ondersteunde versies

Tot de eerste release (v1) krijgt alleen de `main`-branch beveiligingsfixes.

## Ontwerpprincipes

- De firmware maakt geen uitgaande internetverbinding, behalve een update-check die de gebruiker zelf start.
- De ketel houdt zijn eigen beveiliging; TherIQ stuurt alleen setpoints.
- Firmware-updates worden ondertekend; een image dat niet opstart, wordt automatisch teruggedraaid (vanaf fase 1).
