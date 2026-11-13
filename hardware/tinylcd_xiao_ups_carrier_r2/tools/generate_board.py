#!/usr/bin/env python3
"""Generate the routed TinyLCD/XIAO/18650 carrier PCB with KiCad's pcbnew API.

The generated board is intentionally self-contained: every footprint is embedded
in the .kicad_pcb file.  KiCad 10 is used for the final save, zone fill and DRC.
"""

from pathlib import Path
import pcbnew


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tinylcd_xiao_ups_carrier.kicad_pcb"
FP_ROOT = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints")
ADD_SEED_ROUTES = False  # Final routing is imported from Freerouting and checked by KiCad DRC.


def mm(value):
    return pcbnew.FromMM(value)


def vec(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


board = pcbnew.BOARD()
board.SetFileName(str(OUT))
settings = board.GetDesignSettings()
settings.SetCustomTrackWidth(mm(0.25))
settings.SetCustomViaSize(mm(0.80))
settings.SetCustomViaDrill(mm(0.40))
settings.UseCustomTrackViaSize(True)
settings.m_TrackMinWidth = mm(0.15)
settings.m_ViasMinSize = mm(0.60)

# Net classes are exported into the Specctra DSN so the autorouter preserves
# sensible current-path widths instead of treating the battery rails as signals.
net_settings = settings.m_NetSettings
default_class = net_settings.GetDefaultNetclass()
default_class.SetClearance(mm(0.20))
default_class.SetTrackWidth(mm(0.25))
default_class.SetViaDiameter(mm(0.80))
default_class.SetViaDrill(mm(0.40))
power_class = pcbnew.NETCLASS("Power")
power_class.SetClearance(mm(0.20))
power_class.SetTrackWidth(mm(0.60))
power_class.SetViaDiameter(mm(0.90))
power_class.SetViaDrill(mm(0.45))
net_settings.SetNetclass("Power", power_class)
for power_net in (
    "GND", "SYS_5V", "+3V3", "CELL_POS", "CELL_POS_FUSED",
    "PACK_POS", "CELL_NEG", "BATT_SWITCHED",
):
    net_settings.SetNetclassPatternAssignment(power_net, "Power")


def add_net(name):
    net = pcbnew.NETINFO_ITEM(board, name)
    board.Add(net)
    return net


net_names = [
    "GND", "SYS_5V", "+3V3", "D0_RS", "D1_TOUCH_IRQ", "D2_TOUCH_CS",
    "D3_LCD_RST", "D4_TOUCH_DOUT", "D5_SPARE", "D6_SPARE", "D7_SPARE",
    "D8_SCLK", "D9_LCD_CS", "D10_MOSI", "LCD_BL", "LCD_T_IRQ",
    "LCD_T_CS", "LCD_T_DOUT", "CELL_POS", "CELL_POS_FUSED", "PACK_POS",
    "CELL_NEG", "BATT_SWITCHED", "BACKUP_GATE", "TEMP_SENSE", "PROG", "CHRG_N", "STDBY_N",
    "CHRG_LED_A", "DONE_LED_A", "DW01_VCC", "DW01_CS", "DW01_OD",
    "DW01_OC", "FET_COMMON",
]
nets = {name: add_net(name) for name in net_names}


def load_fp(lib, name, ref, value, x, y, rotation=0, layer=pcbnew.F_Cu):
    fp = pcbnew.FootprintLoad(str(FP_ROOT / f"{lib}.pretty"), name)
    if fp is None:
        raise RuntimeError(f"Footprint not found: {lib}:{name}")
    fp.SetReference(ref)
    fp.SetValue(value)
    fp.SetPosition(vec(x, y))
    fp.SetOrientationDegrees(rotation)
    if layer == pcbnew.B_Cu:
        fp.SetLayerAndFlip(layer)
    fp.Reference().SetVisible(False)
    fp.Reference().SetTextSize(vec(1.0, 1.0))
    fp.Reference().SetTextThickness(mm(0.15))
    fp.Value().SetVisible(False)
    board.Add(fp)
    return fp


def assign(fp, mapping):
    pads = {pad.GetNumber(): pad for pad in fp.Pads()}
    for number, net_name in mapping.items():
        if str(number) not in pads:
            raise RuntimeError(f"{fp.GetReference()} has no pad {number}")
        pads[str(number)].SetNet(nets[net_name])
    return fp


# XIAO sockets, USB-C end toward the top board edge.
J1 = assign(load_fp("Connector_PinSocket_2.54mm", "PinSocket_1x07_P2.54mm_Vertical",
                    "J1", "XIAO_LEFT", 10.00, 6.00), {
    1: "D0_RS", 2: "D1_TOUCH_IRQ", 3: "D2_TOUCH_CS", 4: "D3_LCD_RST",
    5: "D4_TOUCH_DOUT", 6: "D5_SPARE", 7: "D6_SPARE",
})
J2 = assign(load_fp("Connector_PinSocket_2.54mm", "PinSocket_1x07_P2.54mm_Vertical",
                    "J2", "XIAO_RIGHT", 25.24, 6.00), {
    1: "SYS_5V", 2: "GND", 3: "+3V3", 4: "D10_MOSI",
    5: "D9_LCD_CS", 6: "D8_SCLK", 7: "D7_SPARE",
})

# 1x11 male header mates with the existing female-female ribbon harness.
J3 = assign(load_fp("Connector_PinHeader_2.54mm", "PinHeader_1x11_P2.54mm_Vertical",
                    "J3", "TINY_LCD_P9", 42.00, 4.00), {
    1: "+3V3", 2: "D0_RS", 3: "D10_MOSI", 4: "D9_LCD_CS",
    5: "D8_SCLK", 6: "D3_LCD_RST", 7: "LCD_BL", 8: "GND",
    9: "LCD_T_IRQ", 10: "LCD_T_CS", 11: "LCD_T_DOUT",
})

# Default-open future-function links.  The counter works with all four open.
SJ1 = assign(load_fp("Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm",
                     "SJ1", "BL_ENABLE", 47.00, 19.24), {1: "LCD_BL", 2: "+3V3"})
SJ2 = assign(load_fp("Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm",
                     "SJ2", "TOUCH_IRQ", 47.00, 24.32), {1: "LCD_T_IRQ", 2: "D1_TOUCH_IRQ"})
SJ3 = assign(load_fp("Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm",
                     "SJ3", "TOUCH_CS", 47.00, 26.86), {1: "LCD_T_CS", 2: "D2_TOUCH_CS"})
SJ4 = assign(load_fp("Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm",
                     "SJ4", "TOUCH_DOUT", 47.00, 29.40), {1: "LCD_T_DOUT", 2: "D4_TOUCH_DOUT"})

# Charger and status components.
U1 = assign(load_fp("Package_SO", "HTSOP-8-1EP_3.9x4.9mm_P1.27mm_EP2.4x3.2mm",
                    "U1", "TP4056", 61.00, 12.00), {
    1: "TEMP_SENSE", 2: "PROG", 3: "GND", 4: "SYS_5V", 5: "PACK_POS",
    6: "STDBY_N", 7: "CHRG_N", 8: "SYS_5V", 9: "GND",
})
R1 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R1", "3.0k", 55.00, 11.35),
            {1: "PROG", 2: "GND"})
R2 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R2", "8.2k", 55.00, 8.80),
            {1: "SYS_5V", 2: "TEMP_SENSE"})
TH1 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "TH1", "NTC_10k_B3950", 55.00, 34.00),
             {1: "TEMP_SENSE", 2: "GND"})
C1 = assign(load_fp("Capacitor_SMD", "C_0805_2012Metric", "C1", "10uF", 55.00, 15.40),
            {1: "SYS_5V", 2: "GND"})
C2 = assign(load_fp("Capacitor_SMD", "C_0805_2012Metric", "C2", "10uF", 67.00, 15.40),
            {1: "PACK_POS", 2: "GND"})
D2 = assign(load_fp("LED_SMD", "LED_0603_1608Metric", "D2", "RED_CHARGE", 70.00, 10.00),
            {1: "CHRG_N", 2: "CHRG_LED_A"})
D3 = assign(load_fp("LED_SMD", "LED_0603_1608Metric", "D3", "GREEN_DONE", 70.00, 13.00),
            {1: "STDBY_N", 2: "DONE_LED_A"})
R3 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R3", "2.2k", 74.00, 10.00),
            {1: "SYS_5V", 2: "CHRG_LED_A"})
R4 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R4", "2.2k", 74.00, 13.00),
            {1: "SYS_5V", 2: "DONE_LED_A"})

# Battery holder and reverse-polarity/high-current path.
B1 = assign(load_fp("Battery", "BatteryHolder_Keystone_1042_1x18650",
                    "B1", "KEYSTONE_1042_18650", 50.00, 48.00), {
    1: "CELL_POS", 2: "CELL_NEG",
})
F1 = assign(load_fp("Fuse", "Fuse_1206_3216Metric", "F1", "1.1A_PTC", 13.00, 34.00),
            {1: "CELL_POS", 2: "CELL_POS_FUSED"})
Q1 = assign(load_fp("Package_TO_SOT_SMD", "SOT-23", "Q1", "AO3401A", 19.00, 34.00),
            {1: "CELL_NEG", 2: "PACK_POS", 3: "CELL_POS_FUSED"})
R5 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R5", "1M", 22.50, 34.00),
            {1: "PACK_POS", 2: "CELL_NEG"})

# Battery-only ON/OFF control.  The 300 mA slide switch drives Q3's gate only;
# Q3 carries the ESP/display load.  D1 is the Seeed-recommended battery diode.
S1 = assign(load_fp("Button_Switch_SMD", "SW_SPDT_CK_JS102011SAQN", "S1", "BATT_ON_OFF", 19.00, 27.00),
            {1: "CELL_NEG", 2: "BACKUP_GATE", 3: "PACK_POS"})
Q3 = assign(load_fp("Package_TO_SOT_SMD", "SOT-23", "Q3", "AO3401A", 26.00, 27.00),
            {1: "BACKUP_GATE", 2: "PACK_POS", 3: "BATT_SWITCHED"})
R8 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R8", "1M", 26.00, 31.00),
            {1: "BACKUP_GATE", 2: "PACK_POS"})
D1 = assign(load_fp("Diode_SMD", "D_SMA", "D1", "SS34", 35.00, 27.00),
            {1: "SYS_5V", 2: "BATT_SWITCHED"})

# DW01A + FS8205A low-side cell protection.
U2 = assign(load_fp("Package_TO_SOT_SMD", "SOT-23-6", "U2", "DW01A", 83.00, 33.50), {
    1: "DW01_OD", 2: "DW01_CS", 3: "DW01_OC",
    5: "DW01_VCC", 6: "CELL_NEG",
})
Q2 = assign(load_fp("Package_TO_SOT_SMD", "SOT-23-6", "Q2", "FS8205A", 91.00, 33.50), {
    1: "CELL_NEG", 2: "FET_COMMON", 3: "GND", 4: "DW01_OC",
    5: "FET_COMMON", 6: "DW01_OD",
})
R6 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R6", "100R", 79.00, 30.50),
            {1: "PACK_POS", 2: "DW01_VCC"})
C3 = assign(load_fp("Capacitor_SMD", "C_0603_1608Metric", "C3", "100nF", 79.00, 34.00),
            {1: "DW01_VCC", 2: "CELL_NEG"})
R7 = assign(load_fp("Resistor_SMD", "R_0603_1608Metric", "R7", "1k", 86.80, 29.50),
            {1: "DW01_CS", 2: "GND"})

# Mechanical mounting holes.
for idx, (x, y) in enumerate(((3.5, 3.5), (96.5, 3.5), (3.5, 56.5), (96.5, 56.5)), 1):
    load_fp("MountingHole", "MountingHole_3.2mm_M3", f"H{idx}", "M3", x, y)


def pad(ref, number):
    fp = next(f for f in board.GetFootprints() if f.GetReference() == ref)
    return next(p for p in fp.Pads() if p.GetNumber() == str(number))


def add_track(net_name, start, end, width=0.25, layer=pcbnew.F_Cu):
    tr = pcbnew.PCB_TRACK(board)
    tr.SetNet(nets[net_name])
    tr.SetLayer(layer)
    tr.SetWidth(mm(width))
    tr.SetStart(vec(*start) if isinstance(start, tuple) else start)
    tr.SetEnd(vec(*end) if isinstance(end, tuple) else end)
    board.Add(tr)


def route(net_name, start_pad, points, end_pad, width=0.25, layer=pcbnew.F_Cu):
    if not ADD_SEED_ROUTES:
        return
    positions = [start_pad.GetPosition()] + [vec(*p) for p in points] + [end_pad.GetPosition()]
    for a, b in zip(positions, positions[1:]):
        add_track(net_name, a, b, width, layer)


# LCD signals: short, ordered and entirely on the front copper.
route("D0_RS", pad("J1", 1), [(30.5, 6.0), (30.5, 6.54)], pad("J3", 2), 0.30)
route("D10_MOSI", pad("J2", 4), [], pad("J3", 3), 0.30)
route("D9_LCD_CS", pad("J2", 5), [], pad("J3", 4), 0.30)
route("D8_SCLK", pad("J2", 6), [], pad("J3", 5), 0.30)
route("D3_LCD_RST", pad("J1", 4), [(31.5, 13.62), (31.5, 16.70)], pad("J3", 6), 0.30, pcbnew.B_Cu)
route("+3V3", pad("J2", 3), [(27.5, 11.08), (27.5, 25.4), (29.0, 25.4)], pad("J3", 1), 0.60, pcbnew.B_Cu)

# Header-to-jumper links and optional future touch routes on the back.
route("LCD_BL", pad("J3", 7), [], pad("SJ1", 1), 0.25)
route("+3V3", pad("SJ1", 2), [(49.5, 19.24), (49.5, 2.0), (42.0, 2.0)], pad("J3", 1), 0.25, pcbnew.B_Cu)
route("LCD_T_IRQ", pad("J3", 9), [], pad("SJ2", 1), 0.25)
route("LCD_T_CS", pad("J3", 10), [], pad("SJ3", 1), 0.25)
route("LCD_T_DOUT", pad("J3", 11), [], pad("SJ4", 1), 0.25)
route("D1_TOUCH_IRQ", pad("J1", 2), [(8.0, 8.54), (8.0, 23.0), (50.0, 23.0)], pad("SJ2", 2), 0.25, pcbnew.B_Cu)
route("D2_TOUCH_CS", pad("J1", 3), [(7.0, 11.08), (7.0, 25.5), (50.0, 25.5)], pad("SJ3", 2), 0.25, pcbnew.B_Cu)
route("D4_TOUCH_DOUT", pad("J1", 5), [(6.0, 16.16), (6.0, 28.0), (50.0, 28.0)], pad("SJ4", 2), 0.25, pcbnew.B_Cu)

# Battery high-current path and USB/battery OR-ing.
route("CELL_POS", pad("B1", 1), [(10.31, 36.0)], pad("F1", 1), 1.00)
route("CELL_POS_FUSED", pad("F1", 2), [(16.0, 34.0)], pad("Q1", 3), 1.00)
route("PACK_POS", pad("Q1", 2), [(22.0, 34.0), (22.0, 27.0)], pad("Q3", 2), 1.00)
route("PACK_POS", pad("Q3", 2), [(23.0, 24.25)], pad("S1", 3), 0.25)
route("PACK_POS", pad("Q3", 2), [(28.0, 31.0)], pad("R8", 2), 0.25)
route("PACK_POS", pad("Q1", 2), [(32.0, 34.0), (32.0, 18.0), (66.0, 18.0)], pad("U1", 5), 0.80)
route("CELL_NEG", pad("S1", 1), [(16.0, 29.0), (25.0, 35.0)], pad("R5", 2), 0.25, pcbnew.B_Cu)
route("BACKUP_GATE", pad("S1", 2), [(22.0, 27.0)], pad("Q3", 1), 0.25)
route("BACKUP_GATE", pad("Q3", 1), [(24.0, 31.0)], pad("R8", 1), 0.25)
route("BATT_SWITCHED", pad("Q3", 3), [(30.0, 27.0)], pad("D1", 2), 1.00)
route("SYS_5V", pad("D1", 1), [(29.0, 23.0), (23.0, 23.0), (23.0, 6.0)], pad("J2", 1), 1.00)
route("SYS_5V", pad("J2", 1), [(23.0, 6.0), (23.0, 23.0), (52.0, 23.0), (52.0, 13.90)], pad("U1", 4), 0.80)

# Charger passives. R1 programs about 400 mA and returns to ground through the pour.
route("PROG", pad("U1", 2), [(56.5, 11.365)], pad("R1", 1), 0.25)
route("SYS_5V", pad("U1", 4), [(55.0, 13.905)], pad("C1", 1), 0.50)
route("PACK_POS", pad("U1", 5), [(67.0, 13.905)], pad("C2", 1), 0.50)
route("TEMP_SENSE", pad("U1", 1), [(56.5, 10.095), (56.5, 8.8)], pad("R2", 2), 0.25)
route("TEMP_SENSE", pad("R2", 2), [], pad("TH1", 1), 0.25)
route("SYS_5V", pad("R2", 1), [(52.0, 8.8), (52.0, 13.9)], pad("U1", 4), 0.25)
route("CHRG_N", pad("U1", 7), [(66.0, 11.365), (66.0, 10.0)], pad("D2", 1), 0.25)
route("STDBY_N", pad("U1", 6), [(67.0, 12.635), (67.0, 13.0)], pad("D3", 1), 0.25)
route("CHRG_LED_A", pad("D2", 2), [], pad("R3", 2), 0.25)
route("DONE_LED_A", pad("D3", 2), [], pad("R4", 2), 0.25)
route("SYS_5V", pad("R3", 1), [(76.0, 10.0), (76.0, 13.9), (63.5, 13.9)], pad("U1", 4), 0.25)
route("SYS_5V", pad("R4", 1), [(76.0, 13.0), (76.0, 13.9), (63.5, 13.9)], pad("U1", 4), 0.25)
route("SYS_5V", pad("U1", 8), [(63.5, 10.095), (63.5, 13.905)], pad("U1", 4), 0.25)

# Reverse-polarity MOSFET gate and pull-up.
route("PACK_POS", pad("Q1", 2), [], pad("R5", 1), 0.25)
route("CELL_NEG", pad("Q1", 1), [], pad("R5", 2), 0.25)
route("CELL_NEG", pad("R5", 2), [(25.0, 35.0), (25.0, 40.0), (88.0, 40.0)], pad("B1", 2), 0.35, pcbnew.B_Cu)

# Protection IC connections and cell-negative current path.
route("CELL_NEG", pad("B1", 2), [(89.69, 37.0)], pad("Q2", 1), 1.00)
route("DW01_OD", pad("U2", 1), [(87.0, 32.23)], pad("Q2", 6), 0.25)
route("DW01_OC", pad("U2", 3), [(87.0, 34.77)], pad("Q2", 4), 0.25)
route("FET_COMMON", pad("Q2", 2), [], pad("Q2", 5), 1.00)
route("DW01_CS", pad("U2", 2), [(86.0, 33.5), (86.0, 29.5)], pad("R7", 1), 0.25)
route("PACK_POS", pad("U1", 5), [(71.0, 18.0), (71.0, 30.5)], pad("R6", 1), 0.25)
route("DW01_VCC", pad("R6", 2), [(80.5, 30.5)], pad("U2", 5), 0.25)
route("DW01_VCC", pad("U2", 5), [(80.5, 34.0)], pad("C3", 1), 0.25)
route("CELL_NEG", pad("C3", 2), [(81.0, 35.5)], pad("U2", 6), 0.25)


def add_zone(net_name, layer):
    zone = pcbnew.ZONE(board)
    zone.SetNet(nets[net_name])
    zone.SetLayer(layer)
    zone.SetLocalClearance(mm(0.25))
    zone.SetMinThickness(mm(0.20))
    zone.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    outline = zone.Outline()
    outline.NewOutline()
    for x, y in ((0.5, 0.5), (99.5, 0.5), (99.5, 59.5), (0.5, 59.5)):
        outline.Append(mm(x), mm(y))
    board.Add(zone)
    return zone


add_zone("GND", pcbnew.F_Cu)
add_zone("GND", pcbnew.B_Cu)


def add_line(x1, y1, x2, y2, layer=pcbnew.Edge_Cuts, width=0.15):
    shape = pcbnew.PCB_SHAPE(board)
    shape.SetShape(pcbnew.SHAPE_T_SEGMENT)
    shape.SetLayer(layer)
    shape.SetWidth(mm(width))
    shape.SetStart(vec(x1, y1))
    shape.SetEnd(vec(x2, y2))
    board.Add(shape)


# 100 x 60 mm, safely inside JLCPCB's low-cost 100 x 100 mm tier.
add_line(0, 0, 100, 0)
add_line(100, 0, 100, 60)
add_line(100, 60, 0, 60)
add_line(0, 60, 0, 0)


def add_text(text, x, y, size=1.2, layer=pcbnew.F_SilkS, rotation=0, bold=False):
    item = pcbnew.PCB_TEXT(board)
    item.SetText(text)
    item.SetPosition(vec(x, y))
    item.SetLayer(layer)
    item.SetTextSize(vec(size, size))
    item.SetTextThickness(mm(max(0.15, size * 0.14)))
    item.SetTextAngleDegrees(rotation)
    item.SetBold(bold)
    if layer == pcbnew.B_SilkS:
        item.SetMirrored(True)
    board.Add(item)


add_text("ESP-YT-SUBS UPS CARRIER  R2", 76, 2.0, 1.15, bold=True)
add_text("ESP-YT SUBS", 84.0, 22.0, 1.20, bold=True)
add_text("USB-C", 17.6, 2.0, 1.0)
add_text("XIAO ESP32-C6", 17.6, 13.0, 1.0, bold=True)
for index, label in enumerate(("D0", "D1", "D2", "D3", "D4", "D5", "D6")):
    add_text(label, 6.6, 6.0 + index * 2.54, 0.8, layer=pcbnew.B_SilkS)
for index, label in enumerate(("5V", "GND", "3V3", "D10", "D9", "D8", "D7")):
    add_text(label, 29.0, 6.0 + index * 2.54, 0.8, layer=pcbnew.B_SilkS)
add_text("P9 PIN 1", 36.0, 2.0, 0.8)
add_text("P9", 36.5, 1.5, 0.8, layer=pcbnew.B_SilkS, bold=True)
for index, label in enumerate(("3V3", "RS", "MOSI", "CS", "SCLK", "RST",
                               "BL", "GND", "T_IRQ", "T_CS", "T_DOUT")):
    add_text(label, 36.5, 4.0 + index * 2.54, 0.8, layer=pcbnew.B_SilkS)
add_text("BL/TOUCH LINKS OPEN", 51.0, 32.5, 0.8, layer=pcbnew.B_SilkS)
add_text("CHG", 70.0, 8.0, 0.8)
add_text("DONE", 70.0, 15.0, 0.8)
add_text("BACKUP", 19.0, 21.5, 0.8)
add_text("ON / OFF", 19.0, 31.0, 0.8)
add_text("18650 3.6/3.7V Li-ion ONLY", 50.0, 43.5, 1.15, bold=True)
add_text("+", 4.5, 48.0, 1.2, bold=True)
add_text("-", 95.5, 48.0, 1.2, bold=True)
add_text("Protected/charged on board - observe polarity", 50.0, 54.5, 0.85)

# Fill both copper planes before saving so Gerbers are immediately usable.
pcbnew.ZONE_FILLER(board).Fill(board.Zones())
pcbnew.SaveBoard(str(OUT), board)
print(OUT)
