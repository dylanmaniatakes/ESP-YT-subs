#!/usr/bin/env python3
"""Correct the routed R1 board's XIAO right-row ordering and mark it R2."""

from pathlib import Path
import pcbnew

ROOT = Path(__file__).resolve().parents[1]
BOARD_PATH = ROOT / "tinylcd_xiao_ups_carrier.kicad_pcb"


def mm(value):
    return pcbnew.FromMM(value)


def vec(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


board = pcbnew.LoadBoard(str(BOARD_PATH))
footprints = {fp.GetReference(): fp for fp in board.GetFootprints()}
j2 = footprints["J2"]
j3 = footprints["J3"]
j1 = footprints["J1"]
nets = {name: board.FindNet(name) for name in (
    "SYS_5V", "GND", "+3V3", "D10_MOSI", "D9_LCD_CS", "D8_SCLK",
    "D7_SPARE", "D3_LCD_RST",
)}

mapping = {
    "1": "SYS_5V", "2": "GND", "3": "+3V3", "4": "D10_MOSI",
    "5": "D9_LCD_CS", "6": "D8_SCLK", "7": "D7_SPARE",
}
for pad in j2.Pads():
    pad.SetNet(nets[mapping[pad.GetNumber()]])


pad_positions = {
    (ref, pad.GetNumber()): pad.GetPosition()
    for ref, fp in (("J1", j1), ("J2", j2), ("J3", j3))
    for pad in fp.Pads()
}


def pad_position(ref, number):
    return pad_positions[(ref, str(number))]


def add_track(net_name, start, end, width, layer):
    track = pcbnew.PCB_TRACK(board)
    track.SetNet(nets[net_name])
    track.SetLayer(layer)
    track.SetWidth(mm(width))
    track.SetStart(start if not isinstance(start, tuple) else vec(*start))
    track.SetEnd(end if not isinstance(end, tuple) else vec(*end))
    board.Add(track)


def route(net_name, points, width, layer):
    for start, end in zip(points, points[1:]):
        add_track(net_name, start, end, width, layer)


def add_via(net_name, x, y, size=0.9, drill=0.45):
    via = pcbnew.PCB_VIA(board)
    via.SetNet(nets[net_name])
    via.SetPosition(vec(x, y))
    via.SetWidth(mm(size))
    via.SetDrill(mm(drill))
    via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    board.Add(via)


def is_at(point, x, y):
    return abs(pcbnew.ToMM(point.x) - x) < 0.001 and abs(pcbnew.ToMM(point.y) - y) < 0.001


# Replace all four display-control routes so the three SPI tracks can fan out
# in parallel on F.Cu and reset can pass underneath on B.Cu.
for item in list(board.GetTracks()):
    net_name = item.GetNetname()
    if (isinstance(item, pcbnew.PCB_VIA) and net_name == "+3V3" and
            is_at(item.GetPosition(), 47.7152, 17.8297)):
        board.Remove(item)
        continue
    if (isinstance(item, pcbnew.PCB_VIA) and net_name == "SYS_5V" and
            is_at(item.GetPosition(), 33.0, 25.4394)):
        board.Remove(item)
        continue
    if isinstance(item, pcbnew.PCB_TRACK) and net_name in {
        "D10_MOSI", "D9_LCD_CS", "D8_SCLK", "D3_LCD_RST"
    }:
        board.Remove(item)
        continue
    if not isinstance(item, pcbnew.PCB_TRACK):
        continue
    # Remove only the obsolete first leg from each affected power pad.
    if net_name == "SYS_5V" and (
        is_at(item.GetStart(), 25.24, 16.16) or is_at(item.GetEnd(), 25.24, 16.16)
        or is_at(item.GetStart(), 33.0, 23.92) or is_at(item.GetEnd(), 33.0, 23.92)
        or is_at(item.GetStart(), 33.0, 25.4394) or is_at(item.GetEnd(), 33.0, 25.4394)
    ):
        board.Remove(item)
    elif net_name == "+3V3" and item.GetLayer() == pcbnew.B_Cu:
        board.Remove(item)
    elif net_name == "GND" and (
        is_at(item.GetStart(), 25.24, 18.70) or is_at(item.GetEnd(), 25.24, 18.70)
    ):
        board.Remove(item)

route("D10_MOSI", [pad_position("J2", 4), pad_position("J3", 3)], 0.30, pcbnew.F_Cu)
route("D9_LCD_CS", [pad_position("J2", 5), pad_position("J3", 4)], 0.30, pcbnew.F_Cu)
route("D8_SCLK", [pad_position("J2", 6), pad_position("J3", 5)], 0.30, pcbnew.F_Cu)
route("D3_LCD_RST", [pad_position("J1", 4), (8.0, 13.62), (8.0, 3.2),
                      (26.0, 3.2)], 0.25, pcbnew.B_Cu)
add_via("D3_LCD_RST", 26.0, 3.2, size=0.8, drill=0.4)
route("D3_LCD_RST", [(26.0, 3.2), (29.0, 3.2)], 0.25, pcbnew.F_Cu)
add_via("D3_LCD_RST", 29.0, 3.2, size=0.8, drill=0.4)
route("D3_LCD_RST", [(29.0, 3.2), (40.5, 3.2), (40.5, 16.7),
                      pad_position("J3", 6)], 0.25, pcbnew.B_Cu)

# Rejoin the corrected power pads to their existing routed networks.
route("SYS_5V", [pad_position("J2", 1), (24.0, 6.0), (24.0, 4.2)],
      0.60, pcbnew.B_Cu)
add_via("SYS_5V", 24.0, 4.2)
route("SYS_5V", [(24.0, 4.2), (24.0, 2.2)], 0.60, pcbnew.F_Cu)
add_via("SYS_5V", 24.0, 2.2)
route("SYS_5V", [(24.0, 2.2), (24.0, 0.9), (60.0, 0.9),
                 (62.6323, 9.0773)], 0.60, pcbnew.B_Cu)
route("+3V3", [pad_position("J2", 3), (27.5, 11.08), (27.5, 2.0),
               (42.0, 2.0), pad_position("J3", 1)], 0.60, pcbnew.B_Cu)

# J2 pin 2 is a through-hole ground pad and connects directly to both pours.
for zone in board.Zones():
    zone.SetIsFilled(False)
pcbnew.ZONE_FILLER(board).Fill(board.Zones())

for drawing in list(board.GetDrawings()):
    if isinstance(drawing, pcbnew.PCB_TEXT):
        if "ESP-YT-SUBS UPS CARRIER" in drawing.GetText():
            drawing.SetText("ESP-YT-SUBS UPS CARRIER  R2")
        elif drawing.GetText() == "ESP-YT SUBS":
            drawing.SetText("ESP-YT SUBS")
            drawing.SetTextSize(vec(1.20, 1.20))
        elif drawing.GetText().startswith("P9:"):
            board.Remove(drawing)

def add_back_label(text, x, y):
    label = pcbnew.PCB_TEXT(board)
    label.SetText(text)
    label.SetPosition(vec(x, y))
    label.SetLayer(pcbnew.B_SilkS)
    label.SetMirrored(True)
    label.SetTextSize(vec(0.80, 0.80))
    label.SetTextThickness(mm(0.15))
    board.Add(label)


for index, text in enumerate(("D0", "D1", "D2", "D3", "D4", "D5", "D6")):
    add_back_label(text, 6.6, 6.0 + index * 2.54)
for index, text in enumerate(("5V", "GND", "3V3", "D10", "D9", "D8", "D7")):
    add_back_label(text, 29.0, 6.0 + index * 2.54)
add_back_label("P9", 36.5, 1.5)
for index, text in enumerate(("3V3", "RS", "MOSI", "CS", "SCLK", "RST",
                              "BL", "GND", "T_IRQ", "T_CS", "T_DOUT")):
    add_back_label(text, 36.5, 4.0 + index * 2.54)

pcbnew.SaveBoard(str(BOARD_PATH), board)
print(BOARD_PATH)
