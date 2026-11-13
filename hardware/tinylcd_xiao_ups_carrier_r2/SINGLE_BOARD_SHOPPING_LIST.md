# Single-board hand-assembly shopping list

Inventory and links were checked on **2026-08-17**. Supplier inventory, prices, and minimum quantities can change. The authoritative machine-readable list is [`manufacturing/single_board_hand_assembly_bom.csv`](manufacturing/single_board_hand_assembly_bom.csv).

## Parts fitted to one PCB

| Qty | Designators | Part | Package | Order selection |
|---:|---|---|---|---|
| 2 | C1, C2 | 10uF 10V X5R | 0805 | [LCSC C2170995](https://www.lcsc.com/product-detail/image/0805ZD106MAT2A_C2170995.html) |
| 1 | C3 | 100nF 50V X7R | 0603 | [LCSC C14663](https://www.lcsc.com/product-detail/C14663.html) |
| 1 | D1 | SS34 3A 40V Schottky | SMA | [LCSC C908680](https://www.lcsc.com/product-detail/C908680.html) |
| 1 | D2 | red LED | 0603 | [LCSC C2286](https://www.lcsc.com/product-detail/C2286.html) |
| 1 | D3 | green LED | 0603 | [LCSC C5336487](https://www.lcsc.com/product-detail/C5336487.html) |
| 1 | F1 | 1.1A hold, 16V resettable PTC | 1206 | [LCSC C7542961](https://www.lcsc.com/product-detail/C7542961.html) |
| 2 | Q1, Q3 | AO3401A P-channel MOSFET | SOT-23 | [LCSC C700954](https://www.lcsc.com/product-image/C700954.html) |
| 1 | Q2 | FS8205A dual N-channel MOSFET | SOT-23-6 | [LCSC C2830320](https://www.lcsc.com/product-detail/mosfets_tech-public-fs8205a_C2830320.html) |
| 1 | R1 | 3.0k 1% | 0603 | [LCSC C118392](https://www.lcsc.com/product-detail/C118392.html) |
| 1 | R2 | 8.2k 0.1% | 0603 | [LCSC C861589](https://www.lcsc.com/product-detail/C861589.html) |
| 2 | R3, R4 | 2.2k 1% | 0603 | [LCSC C4190](https://www.lcsc.com/product-detail/C4190.html) |
| 2 | R5, R8 | 1M 1% | 0603 | [LCSC C22935](https://www.lcsc.com/product-detail/C22935.html) |
| 1 | R6 | 100 ohm 1% | 0603 | [LCSC C216798](https://www.lcsc.com/product-detail/C216798.html) |
| 1 | R7 | 1k 1% | 0603 | [LCSC C21190](https://www.lcsc.com/product-detail/C21190.html) |
| 1 | S1 | JS102011SAQN slide switch | SMD | [LCSC C221660](https://www.lcsc.com/product-detail/C221660.html) |
| 1 | TH1 | 10k B3950 1% NTC | 0603 | [LCSC C49247666](https://www.lcsc.com/product-image/C49247666.html) |
| 1 | U1 | TP4056-42 Li-ion charger | ESOP-8 exposed pad | [LCSC C16581](https://www.lcsc.com/product-detail/C16581.html?s_z=n_TP4056) |
| 1 | U2 | DW01A protection controller | SOT-23-6 | [LCSC C351410](https://www.lcsc.com/product-detail/Battery-Management-ICs_PUOLOP_C351410.html) |

The C1/C2, D3, R1, R2, R6, and F1 selections replace unavailable or depleted earlier choices. R2 is a tighter 0.1% part in the same 0603 footprint; F1 remains 1.1A hold, 16V, and 1206.

## Parts not included in the SMT BOM

| Qty | Board position | Part | Source |
|---:|---|---|---|
| 1 | B1 | Keystone 1042 18650 holder | [DigiKey 36-1042-ND](https://www.digikey.com/en/products/detail/keystone-electronics/1042/2745668) |
| 2 | J1, J2 | 1x7, 2.54mm vertical female sockets | [LCSC C2897370](https://www.lcsc.com/product-detail/Female-Headers_HCTL_C2897370.html) |
| 1 | J3 | 1x11, 2.54mm straight male header | [LCSC C2840011](https://www.lcsc.com/product-detail/pin-headers_xfcn-pz254v-11-11p_C2840011.html) |
| 1 | sockets | Two 1x7 male headers for the XIAO | Supplied with many XIAO boards; verify package contents |
| 1 | module | XIAO ESP32-C6 | [Seeed Studio](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html) |
| 1 | B1 | Samsung 35E flat-top unprotected 18650 | [18650 Battery Store](https://www.18650batterystore.com/products/samsung-35e-18650-3500mah-8a-battery) |
| 4 | H1-H4 | M3 screws and standoffs | Optional |

The display and its existing 1x11 female ribbon cable are reused and are not shopping-list items. Confirm that connector really is 2.54mm pitch before ordering the board or J3.

## What to buy for one board

The `Qty` column above is the exact installed quantity. LCSC often sells resistors, capacitors, LEDs, and small semiconductors in minimum lots, so the CSV includes a practical `Suggested Order Qty` separately. Do not multiply the design quantities by five; the PCB order minimum does not change the number required for the one board you intend to populate.

For the least complicated order, buy all small SMD parts and headers from LCSC, the exact Keystone holder from DigiKey, and the XIAO/cell only if you do not already own them. Prices and minimum order quantities are deliberately not totaled because both inventory and breaks change frequently.

## Hand-soldering reality check

- The 0603 passives and SOT-23 parts are manageable with magnification, fine tweezers, a fine tip, and plenty of flux.
- **U1 is not realistically an iron-only part on this board.** Its exposed ground/thermal pad is hidden underneath and has no bottom-side thermal via for solder access. Use solder paste plus a stencil and hot plate/hot air, then touch up the eight leads with the iron.
- The Keystone 1042 is an SMD holder with alignment posts, not a through-hole holder. Its large tabs and the switch anchors need a larger tip and enough heat.
- Check LED, diode, MOSFET, and IC orientation against the KiCad board before heating them. Place U1, U2, Q1-Q3, and the 0603 parts before the tall headers and battery holder.
- Do not insert an 18650 until all unpowered short checks and the current-limited USB rail test in the main README pass. Never charge an unattended or damaged cell.

This revision's CAD checks pass, but the carrier, charger, protection circuit, and battery switchover have not yet been fabricated or bench-validated.
