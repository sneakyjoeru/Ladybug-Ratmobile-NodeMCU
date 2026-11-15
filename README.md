# ratmobile

![Ratmobile — soldering schema](ratmobile-final.svg)

## Files

- `ratmobile-final.svg` — soldering schema (no PCB)
- `ratmobile_home.ino` — ESP8266 firmware (Arduino project file; NodeMCU 1.0 / ESP-12E) — joins home WiFi, falls back to its own hotspot; set `WIFI_SSID` / `WIFI_PASS` first

## Components

1x [ESP8266 serial port wifi module NodeMCU Lua V3 Internet of Things development board TYPE-C interface CH340](https://www.aliexpress.com/item/1005006889833004.html)

<a href="https://www.aliexpress.com/item/1005006889833004.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/S8985a732d0874c949b39cd9e6aa38039y.png" width="250" alt="NodeMCU V3"></a>

1x [DRV8833 2 Channel DC Motor Driver Module Board 1.5A 3V-10V H-Bridge 4-Wire Step Motor Overcurrent Protection Step Motor Drive](https://www.aliexpress.com/item/1005006370873675.html)

<a href="https://www.aliexpress.com/item/1005006370873675.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sb3f9051e501b44b0a8a473e0977bf083B.jpg" width="250" alt="DRV8833 Motor Driver"></a>

1x **important** — [USB-C 1.2A 2 In 1 Type-C USB TP4056 1A Lithium Battery Charging Protection Board Module Polymer Battery With Temperature Protection](https://www.aliexpress.com/item/1005004981377952.html) — LX-LBC3 board with 5V booster (the booster is an R2R). This is the power board: it boosts the 3.7 V battery up to 5 V **and** charges the battery at the same time.

<a href="https://www.aliexpress.com/item/1005004981377952.html"><img src="tp4056-lx-lbc3.png" width="250" alt="TP4056 LX-LBC3 Charger"></a>

2x [A3-28RPM Micro Mini N20 Gear Motor DC 5V Slow Speed Full Metal Gearbox Reducer Electric Motor DIY Toy](https://www.aliexpress.com/item/1005008129258889.html)

<a href="https://www.aliexpress.com/item/1005008129258889.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/S2b7428cf18dd4f658ac75ef97d2bea0d0.jpg" width="250" alt="N20 Gear Motor"></a>

1x Back Wheel [Nylon, CY-25D Screw Universal Ball Transfer Caster Nylon Ball Transfer Conveyor Roller Wheel](https://www.aliexpress.com/item/1005008484115118.html)

<a href="https://www.aliexpress.com/item/1005008484115118.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sbd5785542f004647a94f6d52abd8cedai.jpg" width="250" alt="CY-25D Ball Transfer Caster"></a>

2x [KCD1-110 Red Super Thin Rocker Switch NO/OFF](https://www.aliexpress.com/item/1005010653861985.html)

<a href="https://www.aliexpress.com/item/1005010653861985.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sd8694afdaba44eecba885c8b1860786eP.jpg" width="250" alt="KCD1-110 Red Rocker Switch"></a>

3x (min) [MMD Princess V5 Ultra Switch Linear Tactile 5Pin 38g](https://www.aliexpress.com/item/1005010534245975.html)

<a href="https://www.aliexpress.com/item/1005010534245975.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/S878ca84b7fd04e09a80d2b3ac2e6c2e2Y.jpg" width="250" alt="MMD Princess V5 Ultra Switch"></a>

Any white 5 V cuttable LED strip — for the status lights in front and back. Just cut it, solder to wires, glue to the case. (No specific listing — any 5 V cuttable white strip works.)

2x M3 screws (8 mm length) + 2x M3 heat inserts — needed for the keyboard fixture: heat inserts go into the case, screws (8 mm M3) attach the keyboard through them.

### Capacitors (from the schema — legs only, no wires)

- **2x 100 nF ceramic** — the two small beige disc capacitors; soldered across the two terminals of each motor (suppresses motor noise).
- **1x 100 µF 16 V** — the small electrolytic cylinder (polarity stripe on the side): `+` on Vin, `−` on the GND pin next to it.
- **1x 1000 µF 16 V** — the larger electrolytic cylinder: `+` on DRV VCC, `−` on DRV GND.
