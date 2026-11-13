# TinyLCD + XIAO ESP32-C6 18650 UPS carrier R2

This is the corrected 100 x 60 mm, two-layer R2 carrier for the subscriber counter. It holds the XIAO ESP32-C6 in removable 2.54 mm sockets, presents the TinyLCD P9 wiring on one straight 1x11 male header, and adds protected single-cell 18650 charging and automatic backup power.

R2 corrects the R1 right-side XIAO socket ordering. With the XIAO component side up and USB-C toward the board's `USB-C` marking, J2 is `VBUS, GND, 3V3, D10, D9, D8, D7` from top to bottom.

The PCB silkscreen includes a neutral project mark. This branding is independent of the firmware's configurable YouTube channel handle.

The backside silkscreen labels every XIAO socket position. J1 is marked `D0` through `D6`; J2 is marked `5V`, `GND`, `3V3`, `D10`, `D9`, `D8`, and `D7`, starting at the USB-C end.

The P9 display header is labeled the same way on the backside, with `3V3`, `RS`, `MOSI`, `CS`, `SCLK`, `RST`, `BL`, `GND`, `T_IRQ`, `T_CS`, and `T_DOUT` printed beside their corresponding pins.

## Status

- KiCad 10 schematic ERC: **0 errors, 0 warnings**
- KiCad 10 PCB DRC: **0 violations, 0 unconnected pads**
- Fully routed two-layer PCB and filled ground planes
- Gerber/drill order ZIP, JLCPCB BOM, and JLCPCB component placement file generated
- Existing display-to-XIAO signal mapping comes from the physically working subscriber counter
- The display plug is assumed to be the photographed 1x11 **2.54 mm Dupont-style** female connector. Check its pitch with a ruler or calipers before ordering; the photos cannot prove the mechanical pitch to manufacturing tolerance.
- **R2 has passed local CAD checks but has not yet been fabricated or electrically bench-tested. Inspect the fabrication preview before ordering.**

## Files

- `tinylcd_xiao_ups_carrier.kicad_pro` — open this project in KiCad
- `tinylcd_xiao_ups_carrier.kicad_sch` — editable schematic
- `tinylcd_xiao_ups_carrier.kicad_pcb` — routed PCB
- `renders/tinylcd_xiao_ups_carrier_schematic.pdf` — printable schematic
- `drc_report.txt` and `erc_report.txt` — final KiCad check reports
- `manufacturing/tinylcd_xiao_ups_carrier_r2_gerbers.zip` — upload this for bare boards
- `manufacturing/jlcpcb_bom.csv` — top-side SMT assembly BOM
- `manufacturing/jlcpcb_cpl.csv` — top-side SMT placement file
- `manufacturing/manual_parts.csv` — parts to solder after JLCPCB assembly
- `manufacturing/single_board_hand_assembly_bom.csv` — one-board shopping BOM with current supplier links
- `manufacturing/lcsc_upload_bom_single_board.csv` — LCSC upload BOM using order quantities that meet the listed minimums
- `manufacturing/digikey_upload_bom_single_board.csv` — DigiKey upload BOM for the exact battery holder
- `SINGLE_BOARD_SHOPPING_LIST.md` — human-readable sourcing and hand-assembly guide

## TinyLCD connector

`J3` is a straight 1x11, 2.54 mm male header. The board silkscreen marks **P9 PIN 1**. Pin 1 is the end nearest the XIAO/USB-C label.

| P9 pin | TinyLCD label | Carrier destination | Default |
|---:|---|---|---|
| 1 | 3V3 | XIAO 3V3 | Connected |
| 2 | RS | XIAO D0 / GPIO0 | Connected |
| 3 | MOSI | XIAO D10 / GPIO18 | Connected |
| 4 | CS | XIAO D9 / GPIO20 | Connected |
| 5 | SCLK | XIAO D8 / GPIO19 | Connected |
| 6 | RST | XIAO D3 / GPIO21 | Connected |
| 7 | BL | SJ1 to 3V3 | Open |
| 8 | GND | GND | Connected |
| 9 | T_IRQ | SJ2 to XIAO D1 | Open |
| 10 | T_CS | SJ3 to XIAO D2 | Open |
| 11 | T_DOUT | SJ4 to XIAO D4 | Open |

Leave `SJ1` open. For touch operation, close `SJ2`, `SJ3`, and `SJ4`, or use temporary jumpers for the equivalent connections: `T_IRQ` to XIAO D1, `T_CS` to D2, and `T_DOUT` to D4. LCD MOSI/D10 and SCLK/D8 are shared with the touch controller.

## Power design

The XIAO USB-C connector remains the normal input. Its 5V rail powers the XIAO/display and a TP4056 single-cell charger. With USB absent, the protected cell feeds the XIAO 5V input through a P-channel load switch and SS34 Schottky diode. When USB returns, its higher voltage automatically takes over without back-feeding the cell.

- TP4056 charge current: approximately 400 mA (`R1 = 3.0k`)
- Charge temperature sensing: 10k B3950 NTC beside the holder
- Cell protection: DW01A + FS8205A (over-charge, over-discharge, and over-current protection)
- Additional input protection: 1.1A resettable fuse and reverse-polarity P-MOSFET
- `S1`: disables battery backup only; USB operation and charging remain available
- `S1` drives `Q3`'s gate, so the display current does not pass through the 300 mA switch contacts
- `D2` red: charging; `D3` green: charge complete

This is seamless backup for short power failures, not a regulated 5V boost supply. On battery, the XIAO 5V pin receives approximately the cell voltage minus the Schottky drop. That follows Seeed's documented external-battery-to-5V-pin diode arrangement, but the finished prototype must still be tested across the full cell discharge range with this particular display.

## JLCPCB order

### Bare boards only

1. Upload `manufacturing/tinylcd_xiao_ups_carrier_r2_gerbers.zip` to JLCPCB.
2. Use: 2 layers, 1.6 mm FR-4, 1 oz copper, green solder mask, and HASL lead-free or ENIG.
3. The detected board size should be 100 x 60 mm. Do not continue if the dimensions or outline differ.

### SMT-assembled boards

1. Start with the same Gerber ZIP and enable top-side PCB assembly.
2. Upload `manufacturing/jlcpcb_bom.csv` and `manufacturing/jlcpcb_cpl.csv`.
   The CPL intentionally uses negative Y coordinates to match KiCad's Gerber
   manufacturing-coordinate convention for this board. Do not convert them to
   positive values before uploading.
3. Confirm every designator maps to the expected LCSC part. Inventory and catalog mappings change, so approve substitutions only after matching value, package, pinout, voltage/current ratings, and polarity.
4. Inspect the graphical placement preview. Pay special attention to `D1`, `D2`, `D3`, `Q1`, `Q2`, `Q3`, `U1`, `U2`, and the switch orientation.
5. `B1`, `J1`, `J2`, and `J3` are intentionally omitted from the SMT BOM; install those manually.

## Hand assembly

1. Do **not** install a cell yet.
2. Solder the two 1x7 female headers into `J1` and `J2`. Use the XIAO with temporary male headers as an alignment jig, but do not heat the XIAO excessively.
3. Solder a straight 1x11 male header into `J3`.
4. Solder the Keystone 1042 holder into `B1`, matching its positive contact to the large board `+` mark.
5. Solder male headers to the XIAO and insert it with USB-C toward the board's `USB-C` marking.
6. Keep `SJ1` open. Initially leave the touch links open for the power test; after the display works, close `SJ2`, `SJ3`, and `SJ4` to enable touch.

## First-power test

Li-ion mistakes can cause fire. Use eye protection, a nonflammable work surface, and a current-limited USB source for the first test. Never use a damaged cell, never short the holder, and never insert an 18650 backwards.

1. With no XIAO, display, USB, or cell connected, check for shorts between `SYS_5V`, `3V3`, `PACK_POS`, and GND.
2. Set `S1` to battery OFF. Connect current-limited USB and confirm about 5V at the XIAO 5V socket and no abnormal heating.
3. Remove USB. Verify the holder polarity with a meter, then insert one reputable 3.6/3.7V 18650 in the marked direction.
4. Reconnect USB. Confirm the red charge LED behavior and monitor `U1`, the cell, and the holder for heat. Stop immediately if anything becomes hot, smells unusual, or draws unexpected current.
5. Install the XIAO only after the rail checks pass. Test the dashboard on USB before attaching the display cable.
6. Attach the ribbon with P9 pin 1 aligned to `J3` pin 1. Confirm 3.3V and GND orientation before power-up.
7. Turn battery backup ON, run from USB, then unplug USB. The counter should remain running. Test again with a partly discharged cell before considering the design validated.

## Regeneration note

`tools/generate_board.py` produces the unrouted placement/netlist board; the committed `.kicad_pcb` also includes the final routed copper. Do not run that script over the finished PCB unless you intend to route it again. `tools/generate_schematic.py` requires the third-party `kicad-sch-api` package.
