# R2 verification record

Run on 2026-08-31 with KiCad 10.0.5.

| Check | Result |
|---|---|
| Schematic ERC | 0 errors, 0 warnings |
| PCB DRC | 0 violations |
| PCB unconnected pads | 0 |
| Schematic-to-PCB net comparison | 93 connected symbol pins compared; 0 unexpected differences |
| BOM-to-CPL reference comparison | 22 SMT references in each; 0 missing |
| Gerber package | 2 copper, 2 mask, 2 silkscreen, top paste, edge cuts, PTH and NPTH drill files |
| Visual output | Top and isometric KiCad renders inspected |

Intentional non-connections are XIAO `D5`, `D6`, and `D7`, plus DW01A test pin 4. The four backlight/touch solder jumpers are routed but open by default. R2's physical J2 order is `VBUS, GND, 3V3, D10, D9, D8, D7` from the USB-C end.

These are CAD checks, not a substitute for fabrication, assembly inspection, electrical bench testing, thermal testing, reverse-polarity testing, or testing the switchover across a complete 18650 discharge cycle.
