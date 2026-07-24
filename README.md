# esphome-novoferm

ESPHome external component voor Novoferm / Tormatic garagepoortmotoren
via de UART "SmartHome"-poort (mini-USB), o.a. Novoferm 423, Novoport IV
en **Tormatic W-600 (L-Door 600)**.

Gebaseerd op de herbouwde `novoferm`-component van
[MartB/esphome](https://github.com/MartB/esphome/tree/novoferm)
(branch `novoferm`), met dank aan @MartB.

## Toevoegingen t.o.v. MartB's versie

- **Native cyclusteller-sensor** (`sensor: platform: novoferm`): leest de
  16-bit teller van afgeronde openingen uit de byte-registers 0x0E
  (hoog) en 0x0F (laag). De waarde ververst automatisch ±3 s na elke
  afgeronde poortbeweging; `update_interval` (default 60min) is alleen
  een periodiek vangnet. Alle verzoeken lopen via de bestaande
  commando-wachtrij van de component.

## Gebruik

```yaml
external_components:
  - source: github://fluppie/esphome-novoferm
    components: [ novoferm ]

uart:
  id: uart_tormatic
  tx_pin: GPIO5
  rx_pin: GPIO23
  baud_rate: 9600

novoferm:
  uart_id: uart_tormatic

cover:
  - platform: novoferm
    id: poort
    name: "Poort"
    device_class: garage
    open_duration: 26s
    close_duration: 23s
    update_interval: 1s
    learn_cycle_times: true

light:
  - platform: novoferm
    name: "Poortverlichting"

sensor:
  - platform: novoferm
    name: "Poort cyclusteller"
```

## Hardware (W-600)

5V-logica op de mini-USB-poort → levelshifter (bv. BSS138) verplicht.
9600 baud 8N1. Zie de protocoldocumentatie voor frame-opbouw,
statuswaarden en valkuilen (echo-mode, duty-limiet 3 cycli/uur).

## Licentie

Zelfde licentie als ESPHome/de upstream-component (C++: GPLv3,
Python: MIT).
