# Tormatic White 600 (W-600) — mini-USB UART-protocol

*Gereverse-engineered 2026-07-24 op een W-600 (typeplaatje 03/2019,
Novoferm tormatic GmbH, Dortmund), verkocht als L-Door 600. Compatibel
met het bekende Novoferm 423/NovoPort-protocol (ESPHome `tormatic` /
MartB `novoferm` component), plus nieuw gedocumenteerde registers.
Werkende ESPHome-component met native cyclusteller:
https://github.com/fluppie/esphome-novoferm*

## Fysieke laag

- Mini-USB (mini-B) bus op de motorkop, zelfde poort als het officiële
  SmartHome WiFi/Homematic-moduul ("Mobility-module", klem P in de
  handleiding).
- Pinout: VBUS = 5V-voeding (ESP32 + shifter werkt er prima op),
  GND, D+/D- = **UART 9600 baud 8N1 op 5V-logica** (level shifter
  verplicht richting 3,3V-microcontrollers).
- Let op: aderkleuren van kabels zijn onbetrouwbaar; bepaal de
  TX/RX-orientatie proefondervindelijk.

## Frame-formaat (big-endian)

```
[seq:2] [len:4] [type:2] [payload:len-2]
```

- `seq`: vrij te kiezen; de motor spiegelt hem in het antwoord.
  (De bestaande componenten gebruiken de hoge seq-byte als
  afzender-tag om antwoorden te matchen.)
- `len`: payloadlengte + 2 (het type-veld telt mee).
- Types: `0x0104` = statusverzoek, `0x0106` = commando,
  `0x0184` = foutantwoord (type | 0x80).

## Commando's (type 0x0106) — payload `00 <doel> 00 <waarde>`

| Doel | Waarde | Werking |
|------|--------|---------|
| 0x0A | 0x00 | Stop / pauze |
| 0x0A | 0x01 | Volledig sluiten |
| 0x0A | 0x02 | Ventilatiestand (~10 cm open) |
| 0x0A | 0x03 | Volledig openen |
| 0x0B | 0x00 | Verlichting uit |
| 0x0B | 0x01 | Verlichting aan |

De motor bevestigt elk geldig commando met een **byte-perfecte echo**.

## Statusverzoeken (type 0x0104) — payload `00 <ID> 00 01`

Antwoord: `[seq][len=5][0x0104][02 <b1> <b2>]`, of bij een
onbekend ID: `[seq][len=3][0x0184][03]` (NACK, foutcode 3).

### Volledige registerkaart (scan 0x00–0xFF, niets daarbuiten antwoordt)

| ID | Antwoord (b1:b2) | Betekenis |
|----|------------------|-----------|
| 0x09 | `00:64` (=100) | Onbekend. **Géén positie**: blijft 100 tijdens dicht, openen, open en sluiten (live geverifieerd) |
| 0x0A | `<status>:00` | **Poortstatus** in b1: 0=PAUSED, 1=CLOSED, 2=VENTILATING, 3=OPENED, 4=OPENING, 5=CLOSING — alle zes live geverifieerd |
| 0x0B | `00:<licht>` | **Lichtstatus** in b2 — volgt ook de automatische bewegingsverlichting incl. nagloeitimer (default 60 s, menu 7 van de aandrijving) |
| 0x0C | `00:00` | Onbekend; blijft 0 tijdens normale bewegingen. Hypothese: diagnosecode (handleiding hst. 7 kent codes 0–9/A/E/u/H); nog niet getest met een veiligheidsstop |
| 0x0D | `00:00` | Onbekend; idem 0x0C |
| 0x0E | `00:<hoog>` | **Cyclusteller, hoog byte** |
| 0x0F | `00:<laag>` | **Cyclusteller, laag byte**. 16-bit totaal (0x0E<<8 \| 0x0F). Telt +1 per **voltooide opening** (register wordt bijgeboekt vlak vóór de status op OPENED springt); ventilatiestand en afgebroken bewegingen tellen niet |
| 0x10 | `24:11` (statisch) | Onbekend (firmwareversie/datum?) |
| overig | NACK 0x0184/03 | 0x00–0x08 en 0x11–0xFF worden niet ondersteund |

De cyclusteller is een officiële functie: handleiding §7.2 toont hem
cijfer-voor-cijfer op het display. §3.2 geeft de context: **max. cycli
totaal 25.000** — de teller is dus direct bruikbaar als
slijtage-/onderhoudssensor (bijv. percentage van de ontwerplevensduur).

**Wat de interface niet biedt:** realtime positie en motorstroom zijn
niet uitleesbaar (de motor gebruikt ze intern voor eindposities en
krachtbegrenzing, maar publiceert ze niet). Positie moet op tijdbasis
geschat worden; stroom desgewenst extern meten (bijv. INA226).

## ⚠ Bekende valkuil 1: de "echo-modus"

De W-600 kan in een toestand raken waarin hij **elk frame letterlijk
echoot** (ook statusverzoeken) en soms commando's negeert. Waargenomen
na kortsluitingen op de datalijnen en/of series ongeldige frames.
Symptomen in ESPHome: `gate state Unknown`, of (stock component)
`Header specifies payload size 4 but size of StatusReply is 3`.

**Oplossing: power cycle van de motor** (30 s van het net). Daarna
gedraagt hij zich weer volledig protocolconform. Dit verklaart mogelijk
ook de "always closed"-meldingen bij andere gebruikers (esphome issue
#13201, vergelijkbare Reddit-posts).

## ⚠ Bekende valkuil 2: ongeldige frames

Scans met ongeldige berichttypes kunnen de interface vergrendelen
(alleen een power cycle helpt). Beperk verkeer tot 0x0104/0x0106 met
correcte lengtes. Statusverzoeken naar onbekende ID's zijn wél veilig
(nette NACK).

## ⚠ ESPHome-valkuil: `dummy_receiver`

Wie zonder leescomponent (alleen `uart.debug.sequence`) meeluistert,
MOET `dummy_receiver: true` zetten — anders wordt de RX-buffer nooit
geleegd en lijkt de motor te zwijgen. Omgekeerd: mét een echte
leescomponent (novoferm/tormatic) mag `dummy_receiver` juist NIET aan
(twee lezers vechten om dezelfde bytes). Minstens één eerder gemelde
"motor is doof"-diagnose in deze sessie bleek deze zelfgemaakte fout.

## Stabiliteit en polling

- Getest stabiel op een **ESP32-WROOM-32** (Wemos D1 mini ESP32):
  zowel rustige polling (status per 10 s) als continue 1 Hz-polling
  van MartB's component, honderden frames zonder één gemist antwoord,
  reply-latency constant 60–90 ms, ook tijdens bewegingen en scans.
- Een eerdere opstelling met een **ESP32-C3** vertoonde herhaaldelijk
  progressief tragere antwoorden tot volledige stilte (alleen motor-
  power-cycle hielp). Deels bleek dat de dummy_receiver-valkuil
  hierboven; of de C3 zelf óók een aandeel had is niet definitief
  vastgesteld. Advies: klassieke ESP32 werkt aantoonbaar betrouwbaar.
- Polltempo's van bekende implementaties: officiële ESPHome `tormatic`
  300 ms; MartB oud (b1c8b9d) 1 s (poort+licht); MartB herbouwde
  `novoferm`-branch default 300 ms, configureerbaar, met nette
  commando-wachtrij (RECEIVE_TIMEOUT 50 ms, reply-timeout 500 ms).

## Duty cycle

Officieel (handleiding §3.2): **max. 3 cycli/uur, 10 cycli/dag**,
25.000 cycli totaal. Bij intensief testen kan de aandrijving tijdelijk
weigeren (thermische beveiliging); de interface blijft dan reageren.

## Werkende ESPHome-setup (referentie)

- Wemos/LOLIN D1 mini ESP32 (WROOM-32) + 4-kanaals BSS138 shifter,
  gevoed uit de 5V van de mini-USB-poort.
- Component: `github://fluppie/esphome-novoferm` — MartB's herbouwde
  novoferm-component (cover met positie-estimatie + learn_cycle_times,
  licht, ventilatie-binary-sensor) uitgebreid met een **native
  cyclusteller-sensorplatform** (ID 0x0E/0x0F via de commando-wachtrij,
  automatische refresh ±3 s na elke voltooide beweging).
- De stock `tormatic` component werkt ook (alleen cover, geen licht).
