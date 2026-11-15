<p align="center"><b><a href="https://makerworld.com/en/models/3621145-ladybug-ratmobile-v2">Download project 3MF at https://makerworld.com/en/models/3621145-ladybug-ratmobile-v2</a></b></p>

# Ladybug Ratmobile V2

<p align="center"><img src="https://github.com/sneakyjoeru/Ladybug-Ratmobile-NodeMCU/releases/download/media/ratmobile-preview.jpg" width="700" alt="Ladybug Ratmobile V2 — finished robot"></p>

<p align="center"><img src="ratmobile-final.svg" alt="Ratmobile — soldering schema"></p>

## Files

- `ratmobile-final.svg` — soldering schema (no PCB)
- `ratmobile_home.ino` — ESP8266 firmware (Arduino project file; NodeMCU 1.0 / ESP-12E) — joins home WiFi, falls back to its own hotspot; set `WIFI_SSID` / `WIFI_PASS` first

## WiFi setup

The robot serves a control page with W A S D buttons and speed sliders.

1. In `ratmobile_home.ino` set `WIFI_SSID` and `WIFI_PASS` (top of the sketch) to your network — **2.4 GHz only**, the ESP8266 cannot see 5 GHz networks.
2. Flash the board, then open **http://ratmobile.local** from any device on the same network. The serial monitor (115200) also prints the IP your router gave it.
3. If the home network can't be reached within 20 seconds, the robot starts its own hotspot — SSID **Ratmobile**, password **ratmobile123890**, control page at **http://192.168.4.1**. Set `FALLBACK_HOTSPOT` to `false` to keep retrying the home network forever instead.
4. Firmware updates over WiFi: **http://ratmobile.local/update** (anyone on the network can open that page).
5. The physical keys A / W / D on the lid keep working whether or not WiFi is connected.

## Components

1x [ESP8266 serial port wifi module NodeMCU Lua V3 Internet of Things development board TYPE-C interface CH340](https://www.aliexpress.com/item/1005006889833004.html)

<a href="https://www.aliexpress.com/item/1005006889833004.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/S80f841ff315047c5a6a749f46bf6ecddc.png" width="250" alt="NodeMCU V3"></a>

1x [DRV8833 2 Channel DC Motor Driver Module Board 1.5A 3V-10V H-Bridge 4-Wire Step Motor Overcurrent Protection Step Motor Drive](https://www.aliexpress.com/item/1005006370873675.html)

<a href="https://www.aliexpress.com/item/1005006370873675.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sb3f9051e501b44b0a8a473e0977bf083B.jpg" width="250" alt="DRV8833 Motor Driver"></a>

1x **important** — [USB-C 1.2A 2 In 1 Type-C USB TP4056 1A Lithium Battery Charging Protection Board Module Polymer Battery With Temperature Protection](https://www.aliexpress.com/item/1005004981377952.html) — LX-LBC3 board with 5V booster (the booster is an R2R). This is the power board: it boosts the 3.7 V battery up to 5 V **and** charges the battery at the same time.

<a href="https://www.aliexpress.com/item/1005004981377952.html"><img src="https://github.com/sneakyjoeru/Ladybug-Ratmobile-NodeMCU/releases/download/media/tp4056-lx-lbc3.png" width="250" alt="TP4056 LX-LBC3 Charger"></a>

1x 18650 cell, 3500 mAh or similar — the battery the LX-LBC3 charges and boosts. (No specific listing — any good-quality 18650 of ~3500 mAh works.) It needs a holder: either buy a cell adapter with wires, or print one — [Flexing battery holders with integrated spring](https://www.thingiverse.com/thing:456900) by enif, the `flexbatter18650Px1` remix we use — and wrap the wires in the print to save some money.

2x [A3-28RPM Micro Mini N20 Gear Motor DC 5V Slow Speed Full Metal Gearbox Reducer Electric Motor DIY Toy](https://www.aliexpress.com/item/1005008129258889.html)

<a href="https://www.aliexpress.com/item/1005008129258889.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sd6bb64dc50a34045ac647931032f74561.jpg" width="250" alt="N20 Gear Motor"></a>

1x Back Wheel [Nylon, CY-25D Screw Universal Ball Transfer Caster Nylon Ball Transfer Conveyor Roller Wheel](https://www.aliexpress.com/item/1005008484115118.html)

<a href="https://www.aliexpress.com/item/1005008484115118.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sbd5785542f004647a94f6d52abd8cedai.jpg" width="250" alt="CY-25D Ball Transfer Caster"></a>

2x [KCD1-110 Red Super Thin Rocker Switch NO/OFF](https://www.aliexpress.com/item/1005010653861985.html)

<a href="https://www.aliexpress.com/item/1005010653861985.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/Sd8694afdaba44eecba885c8b1860786eP.jpg" width="250" alt="KCD1-110 Red Rocker Switch"></a>

3x (min) [MMD Princess V5 Ultra Switch Linear Tactile 5Pin 38g](https://www.aliexpress.com/item/1005010534245975.html) — the lighter the activation, the better.

<a href="https://www.aliexpress.com/item/1005010534245975.html"><img src="https://ae-pic-a1.aliexpress-media.com/kf/S878ca84b7fd04e09a80d2b3ac2e6c2e2Y.jpg" width="250" alt="MMD Princess V5 Ultra Switch"></a>

Any white 5 V cuttable LED strip — for the status lights in front and back. Just cut it, solder to wires, glue to the case. (No specific listing — any 5 V cuttable white strip works.)

2x M3 screws (8 mm length) + 2x M3 heat inserts — needed for the keyboard fixture: heat inserts go into the case, screws (8 mm M3) attach the keyboard through them.

### Capacitors (from the schema — legs only, no wires)

- **2x 100 nF ceramic** — the two small beige disc capacitors; soldered across the two terminals of each motor (suppresses motor noise).
- **1x 100 µF 16 V** — the small electrolytic cylinder (polarity stripe on the side): `+` on Vin, `−` on the GND pin next to it.
- **1x 1000 µF 16 V** — the larger electrolytic cylinder: `+` on DRV VCC, `−` on DRV GND.

## Assembly

All printed parts fit each other freely — no glue and no fasteners except where noted.

- **Bottom case (green, electronics) + cabin floor (any colour you print it in)** — connected by the back wheel's screw: the caster's screw goes through the floor into the bottom case.
- **Cabin walls and roof** — held by gravity only; they just rest in place.
- **Cabin floor** — can be installed without unscrewing the keyboard: a slightly tricky slide, but very easy to learn.
- **Wiring** — the build needs a few free wires. For the optional cable management — any glue; for the lights — any tape to hold them in place.

**Power and flashing**

- Charging goes through the TP4056 module with the booster (USB-C) and works in both positions of the power switch.
- For flashing, the NodeMCU must be connected while the power switch is **off**.

## 3D models used

- [Offroad Tire 110](https://www.thingiverse.com/thing:2119923) by Martimo — the wheels
- [Flexing battery holders with integrated spring](https://www.thingiverse.com/thing:456900) by enif — the 18650 battery holder
## License

Free to build, modify and use for personal, non-commercial purposes. If you repost the design, the STLs, the code or the photos — credit the author and link back to this repository. **Not for sale.**

All derivatives must be published on MakerWorld as a remix of the original: https://makerworld.com/en/models/3621145

Licensed under [Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)](LICENSE).
