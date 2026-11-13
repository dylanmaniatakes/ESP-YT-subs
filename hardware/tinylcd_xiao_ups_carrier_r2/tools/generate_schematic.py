#!/usr/bin/env python3
"""Generate the editable KiCad schematic for the TinyLCD/XIAO UPS carrier.

Requires: pip install kicad-sch-api
Set KICAD_SYMBOL_DIR to KiCad's installed `symbols` directory on nonstandard
installations.  Each connection is named explicitly to keep the three circuit
blocks readable on one sheet and to mirror the PCB net names.
"""

from pathlib import Path
import kicad_sch_api as ksa
from kicad_sch_api.core.pin_utils import get_component_pin_info


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tinylcd_xiao_ups_carrier.kicad_sch"

sch = ksa.create_schematic("TinyLCD XIAO ESP32-C6 UPS Carrier R2")


def add(lib_id, ref, value, pos, footprint="", rotation=0):
    # Symbols and their wire stubs must stay on KiCad's 50 mil connection grid.
    grid = 1.27
    pos = (round(pos[0] / grid) * grid, round(pos[1] / grid) * grid)
    component = sch.components.add(
        lib_id=lib_id,
        reference=ref,
        value=value,
        position=pos,
        footprint=footprint,
    )
    if rotation:
        component.rotate(rotation)
    return component


def connect(component, mapping):
    """Attach readable short wire stubs and local labels to symbol pins."""
    for pin, net in mapping.items():
        pin_position, pin_rotation = get_component_pin_info(component, str(pin))
        distance = 5.08
        direction = {
            0: (-distance, 0),
            90: (0, distance),
            180: (distance, 0),
            270: (0, -distance),
        }[int(pin_rotation) % 360]
        endpoint = (pin_position.x + direction[0], pin_position.y + direction[1])
        sch.add_wire((pin_position.x, pin_position.y), endpoint)
        sch.add_label(text=net, position=endpoint,
                      rotation=(pin_rotation + 180) % 360, size=0.55)


def no_connect(component, *pins):
    for pin in pins:
        marker = sch.add_label(text="__NC_MARKER__", pin=(component.reference, str(pin)), size=0.8)
        label = sch.labels.get(marker)
        sch.no_connects.add(position=label.position)
        sch.labels.remove(marker)


sch.add_text("DISPLAY + REMOVABLE XIAO ESP32-C6", (30, 25), size=1.5, bold=True)
J1 = add("Connector_Generic:Conn_01x07", "J1", "XIAO_LEFT", (42, 52),
         "Connector_PinSocket_2.54mm:PinSocket_1x07_P2.54mm_Vertical")
connect(J1, {1:"D0_RS", 2:"D1_TOUCH_IRQ", 3:"D2_TOUCH_CS", 4:"D3_LCD_RST",
             5:"D4_TOUCH_DOUT"})
no_connect(J1, 6, 7)
J2 = add("Connector_Generic:Conn_01x07", "J2", "XIAO_RIGHT", (88, 52),
         "Connector_PinSocket_2.54mm:PinSocket_1x07_P2.54mm_Vertical")
# Physical top-to-bottom order with the XIAO component side up and USB-C
# toward the board's USB-C silkscreen: VBUS, GND, 3V3, D10, D9, D8, D7.
connect(J2, {1:"SYS_5V", 2:"GND", 3:"+3V3", 4:"D10_MOSI",
             5:"D9_LCD_CS", 6:"D8_SCLK"})
no_connect(J2, 7)
J3 = add("Connector_Generic:Conn_01x11", "J3", "TINY_LCD_P9", (137, 52),
         "Connector_PinHeader_2.54mm:PinHeader_1x11_P2.54mm_Vertical")
connect(J3, {1:"+3V3", 2:"D0_RS", 3:"D10_MOSI", 4:"D9_LCD_CS", 5:"D8_SCLK",
             6:"D3_LCD_RST", 7:"LCD_BL", 8:"GND", 9:"LCD_T_IRQ",
             10:"LCD_T_CS", 11:"LCD_T_DOUT"})

for index, (value, y, left, right) in enumerate((
    ("BL_ENABLE", 35, "LCD_BL", "+3V3"),
    ("TOUCH_IRQ", 45, "LCD_T_IRQ", "D1_TOUCH_IRQ"),
    ("TOUCH_CS", 55, "LCD_T_CS", "D2_TOUCH_CS"),
    ("TOUCH_DOUT", 65, "LCD_T_DOUT", "D4_TOUCH_DOUT"),
), 1):
    sj = add("Jumper:SolderJumper_2_Open", f"SJ{index}", value, (185, y),
             "Jumper:SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm")
    connect(sj, {1:left, 2:right})


sch.add_text("USB CHARGER + STATUS", (30, 88), size=1.5, bold=True)
U1 = add("Battery_Management:TP4056-42-ESOP8", "U1", "TP4056", (75, 111),
         "Package_SO:HTSOP-8-1EP_3.9x4.9mm_P1.27mm_EP2.4x3.2mm")
connect(U1, {1:"TEMP_SENSE", 2:"PROG", 3:"GND", 4:"SYS_5V", 5:"PACK_POS",
             6:"STDBY_N", 7:"CHRG_N", 8:"SYS_5V", 9:"GND"})

charger_parts = (
    ("Device:R", "R1", "3.0k (~400mA)", (118, 96), {1:"PROG", 2:"GND"},
     "Resistor_SMD:R_0603_1608Metric"),
    ("Device:R", "R2", "8.2k", (118, 106), {1:"SYS_5V", 2:"TEMP_SENSE"},
     "Resistor_SMD:R_0603_1608Metric"),
    ("Device:Thermistor_NTC", "TH1", "NTC 10k B3950", (118, 116),
     {1:"TEMP_SENSE", 2:"GND"}, "Resistor_SMD:R_0603_1608Metric"),
    ("Device:C", "C1", "10uF", (151, 96), {1:"SYS_5V", 2:"GND"},
     "Capacitor_SMD:C_0805_2012Metric"),
    ("Device:C", "C2", "10uF", (151, 116), {1:"PACK_POS", 2:"GND"},
     "Capacitor_SMD:C_0805_2012Metric"),
    ("Device:R", "R3", "2.2k", (184, 96), {1:"SYS_5V", 2:"CHRG_LED_A"},
     "Resistor_SMD:R_0603_1608Metric"),
    ("Device:LED", "D2", "RED CHARGE", (211, 96), {1:"CHRG_N", 2:"CHRG_LED_A"},
     "LED_SMD:LED_0603_1608Metric"),
    ("Device:R", "R4", "2.2k", (184, 116), {1:"SYS_5V", 2:"DONE_LED_A"},
     "Resistor_SMD:R_0603_1608Metric"),
    ("Device:LED", "D3", "GREEN DONE", (211, 116), {1:"STDBY_N", 2:"DONE_LED_A"},
     "LED_SMD:LED_0603_1608Metric"),
)
for lib, ref, value, pos, mapping, footprint in charger_parts:
    part = add(lib, ref, value, pos, footprint)
    connect(part, mapping)


sch.add_text("18650 PROTECTION + BACKUP POWER PATH", (30, 130), size=1.5, bold=True)
B1 = add("Device:Battery_Cell", "B1", "18650 Li-ion", (40, 150),
         "Battery:BatteryHolder_Keystone_1042_1x18650")
connect(B1, {1:"CELL_POS", 2:"CELL_NEG"})
F1 = add("Device:Polyfuse", "F1", "1.1A PTC", (65, 145),
         "Fuse:Fuse_1206_3216Metric")
connect(F1, {1:"CELL_POS", 2:"CELL_POS_FUSED"})
Q1 = add("Transistor_FET:AO3401A", "Q1", "AO3401A", (90, 145),
         "Package_TO_SOT_SMD:SOT-23")
connect(Q1, {1:"CELL_NEG", 2:"PACK_POS", 3:"CELL_POS_FUSED"})
R5 = add("Device:R", "R5", "1M", (113, 145), "Resistor_SMD:R_0603_1608Metric")
connect(R5, {1:"PACK_POS", 2:"CELL_NEG"})
S1 = add("Switch:SW_SPDT", "S1", "BACKUP ON/OFF", (135, 145),
         "Button_Switch_SMD:SW_SPDT_CK_JS102011SAQN")
connect(S1, {1:"CELL_NEG", 2:"BACKUP_GATE", 3:"PACK_POS"})
Q3 = add("Transistor_FET:AO3401A", "Q3", "AO3401A", (160, 145),
         "Package_TO_SOT_SMD:SOT-23")
connect(Q3, {1:"BACKUP_GATE", 2:"PACK_POS", 3:"BATT_SWITCHED"})
R8 = add("Device:R", "R8", "1M", (160, 160), "Resistor_SMD:R_0603_1608Metric")
connect(R8, {1:"BACKUP_GATE", 2:"PACK_POS"})
D1 = add("Diode:SS34", "D1", "SS34", (180, 145), "Diode_SMD:D_SMA")
connect(D1, {1:"SYS_5V", 2:"BATT_SWITCHED"})

U2 = add("Battery_Management:DW01A", "U2", "DW01A", (205, 150),
         "Package_TO_SOT_SMD:SOT-23-6")
connect(U2, {1:"DW01_OD", 2:"DW01_CS", 3:"DW01_OC", 5:"DW01_VCC", 6:"CELL_NEG"})
no_connect(U2, 4)
Q2 = add("Connector_Generic:Conn_01x06", "Q2", "FS8205A (pin-explicit)", (245, 150),
         "Package_TO_SOT_SMD:SOT-23-6")
connect(Q2, {1:"CELL_NEG", 2:"FET_COMMON", 3:"GND", 4:"DW01_OC",
             5:"FET_COMMON", 6:"DW01_OD"})
R6 = add("Device:R", "R6", "100R", (200, 160), "Resistor_SMD:R_0603_1608Metric")
connect(R6, {1:"PACK_POS", 2:"DW01_VCC"})
C3 = add("Device:C", "C3", "100nF", (225, 160), "Capacitor_SMD:C_0603_1608Metric")
connect(C3, {1:"DW01_VCC", 2:"CELL_NEG"})
R7 = add("Device:R", "R7", "1k", (250, 160), "Resistor_SMD:R_0603_1608Metric")
connect(R7, {1:"DW01_CS", 2:"GND"})

# ERC source declarations for rails that enter through the XIAO/USB connector
# or through passive battery-protection elements.
for index, (net, y) in enumerate((
    ("SYS_5V", 92), ("GND", 102), ("TEMP_SENSE", 112),
    ("DW01_VCC", 122), ("CELL_NEG", 132),
), 1):
    flag = add("power:PWR_FLAG", f"#FLG0{index}", "PWR_FLAG", (260, y))
    connect(flag, {1:net})

sch.add_text("Switch drives Q3 gate only; battery feeds XIAO 5V through SS34 and USB takes priority.",
             (105, 171), size=1.0)
sch.save(OUT)
print(OUT)
