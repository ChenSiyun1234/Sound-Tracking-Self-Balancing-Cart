#!/usr/bin/env python3
"""
gen_pinout.py - generate a board "pinout card": the board drawn with EVERY pin of its
two headers labelled, and each pin that is used in the project tagged with a colour-coded
connection (signal name; colour = which module it wires to). Renders SVG -> PNG via PyMuPDF.

INPUT = a BOARD dict (title, chip, the top/bottom pin-label rows, an optional side header,
and a `conn` map of (header,index) -> (module, signal)). Add a board + call render() to make
a new diagram for any board. No external draw tools needed (PyMuPDF rasterises the SVG).

Usage:  python gen_pinout.py
"""
import fitz   # PyMuPDF

# ---- module colour palette (colour identifies the module; signal text is on the tag) ----
MODCOL = {
    "MPU-6050":  "#2980b9",   # blue
    "TB6612":    "#c0392b",   # red
    "Encoders":  "#27ae60",   # green
    "HC-05":     "#8e44ad",   # purple
    "HC-SR04":   "#16a085",   # teal
    "OLED":      "#e67e22",   # orange
    "UART dbg":  "#7f8c8d",   # grey
    "Power":     "#d4a017",   # gold (5V / 3V3)
    "GND":       "#34495e",   # slate
    "LED":       "#e84393",   # pink
}

# =================================== board definitions ===================================
STM32 = {
    "title":   "STM32F103C8T6 \"Blue Pill\" — Pinout & Project Connections",
    "subtitle":"Every header pin is labelled; coloured tags show what each used pin wires to (colour = module).",
    "chip":    "STM32F103",
    "top":    ["G","G","3.3","R","B11","B10","B1","B0","A7","A6","A5","A4","A3","A2","A1","A0","C15","C14","C13","VB"],
    "bottom": ["B12","B13","B14","B15","A8","A9","A10","A11","A12","A15","B3","B4","B5","B6","B7","B8","B9","5V","G","3.3"],
    "side":   ("SWD", ["GND","DCLK·A14","DIO·A13","3.3"]),
    "conn": {
        ("top",0):("GND","GND"), ("top",1):("GND","GND"), ("top",2):("Power","3V3"),
        ("top",4):("MPU-6050","SDA"), ("top",5):("MPU-6050","SCL"),
        ("top",7):("HC-SR04","TRIG"), ("top",10):("MPU-6050","INT"),
        ("top",11):("TB6612","STBY"), ("top",12):("HC-05","BT-RX"), ("top",13):("HC-05","BT-TX"),
        ("top",18):("LED","LED"),
        ("bottom",0):("TB6612","AIN1"), ("bottom",1):("TB6612","AIN2"),
        ("bottom",2):("TB6612","BIN1"), ("bottom",3):("TB6612","BIN2"),
        ("bottom",4):("TB6612","PWMA"), ("bottom",5):("UART dbg","TX"), ("bottom",6):("UART dbg","RX"),
        ("bottom",7):("TB6612","PWMB"), ("bottom",8):("HC-SR04","ECHO"),
        ("bottom",9):("Encoders","E1A"), ("bottom",10):("Encoders","E1B"),
        ("bottom",11):("Encoders","E2A"), ("bottom",12):("Encoders","E2B"),
        ("bottom",13):("OLED","GND"), ("bottom",14):("OLED","VCC"),
        ("bottom",15):("OLED","SCL"), ("bottom",16):("OLED","SDA"),
        ("bottom",17):("Power","5V"), ("bottom",18):("GND","GND"), ("bottom",19):("Power","3V3"),
    },
}

# CC3200 LaunchPad - its used pins laid out as two rows (PIN_xx device pins) in the same style.
CC3200 = {
    "title":   "CC3200 LaunchPad — Pinout & Project Connections",
    "subtitle":"Device pins (PIN_xx) used by the project; coloured tags show the module each wires to.",
    "chip":    "CC3200",
    "top":    ["P01","P02","P03","P04","P05","P07","P08","P15","P18","P55"],
    "bottom": ["P57","P45","P50","P53","P63","P64","P61","P62","3.3","GND"],
    "side":   None,
    "conn": {
        ("top",0):("MPU-6050","SCL"), ("top",1):("MPU-6050","SDA"),
        ("top",4):("OLED","SCK"), ("top",5):("OLED","MOSI"),
        ("top",8):("OLED","RST"), ("top",9):("UART dbg","TX"),
        ("bottom",0):("UART dbg","RX"),
        ("bottom",1):("TB6612","BIN2"), ("bottom",2):("TB6612","BIN1"),
        ("bottom",3):("TB6612","AIN2"), ("bottom",4):("TB6612","AIN1"),
        ("bottom",5):("TB6612","PWM"), ("bottom",6):("OLED","D/C"), ("bottom",7):("OLED","CS"),
        ("bottom",8):("Power","3V3"), ("bottom",9):("GND","GND"),
    },
}

# ======================================== renderer ========================================
def render(board, out_png, scale=1.7):
    e = []
    def R(x,y,w,h,fill,stroke="#333",sw=2,rx=6):
        e.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
    def L(x1,y1,x2,y2,c,sw=2):
        e.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{c}" stroke-width="{sw}"/>')
    def C(cx,cy,r,fill,stroke="#666",sw=1.5):
        e.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
    def T(x,y,s,size=13,fill="#111",w="normal",anc="middle"):
        e.append(f'<text x="{x:.1f}" y="{y:.1f}" font-family="Helvetica,Arial,sans-serif" font-size="{size}" fill="{fill}" font-weight="{w}" text-anchor="{anc}">{s}</text>')

    top, bot = board["top"], board["bottom"]
    n = max(len(top), len(bot))
    PITCH = 66
    x0 = 150
    Wd = x0*2 + (n-1)*PITCH                       # canvas width
    bx0, bx1 = x0-26, x0+(n-1)*PITCH+26           # board x span
    by0, by1 = 300, 470                           # board y span
    H = 760

    e.append(f'<rect x="0" y="0" width="{Wd}" height="{H}" fill="#ffffff"/>')
    T(Wd/2, 40, board["title"], 23, "#111", "bold")
    T(Wd/2, 64, board["subtitle"], 13.5, "#666")

    # board body + light "silk" decoration (chip, USB, reset, crystal) for resemblance
    R(bx0, by0, bx1-bx0, by1-by0, "#0c5c2e", "#0a4a25", 3, 10)          # green PCB
    chcx, chcy, chr = (bx0+bx1)/2, (by0+by1)/2, 52
    e.append(f'<polygon points="{chcx:.0f},{chcy-chr} {chcx+chr:.0f},{chcy:.0f} {chcx:.0f},{chcy+chr} {chcx-chr:.0f},{chcy:.0f}" fill="#0c5c2e" stroke="#cfe8d8" stroke-width="2"/>')
    T(chcx, chcy-2, board["chip"], 13, "#eaf7ef", "bold")
    T(chcx, chcy+14, "U2", 11, "#a9d6bd")
    R(bx0+10, chcy-22, 34, 44, "#0c5c2e", "#cfe8d8", 1.5, 3); T(bx0+27, chcy+4, "USB", 9.5, "#cfe8d8")   # USB
    R(bx1-46, chcy-12, 30, 24, "#0c5c2e", "#cfe8d8", 1.5, 3); T(bx1-31, chcy+4, "RST", 9, "#cfe8d8")     # reset
    R(chcx+chr+18, chcy-9, 40, 18, "#0c5c2e", "#cfe8d8", 1.5, 3); T(chcx+chr+38, chcy+4, "8MHz", 8.5, "#cfe8d8")  # crystal

    def pin_x(i): return x0 + i*PITCH

    def draw_row(pins, header, ypad, name_dy, tag_dir):
        # tag_dir = -1 (up, top header) or +1 (down, bottom header)
        near = ypad + tag_dir*42
        far  = ypad + tag_dir*92
        for i, lbl in enumerate(pins):
            x = pin_x(i)
            C(x, ypad, 7, "#d9d9d9")                                   # solder pad (on the board edge)
            T(x, ypad + name_dy, lbl, 13, "#eaf7ef", "bold")          # pin name INSIDE the green PCB
            key = (header, i)
            if key in board["conn"]:
                mod, sig = board["conn"][key]
                col = MODCOL[mod]
                ty = far if (i % 2) else near                         # stagger to avoid overlap
                L(x, ypad, x, ty, col, 2.2)                           # leader from pad out to the tag
                tw, th = 56, 22
                R(x-tw/2, ty-(th if tag_dir<0 else 0), tw, th, col, col, 0, 5)
                T(x, ty - (th if tag_dir<0 else 0) + 15, sig, 11.5, "#ffffff", "bold")

    draw_row(top, "top", by0,  22, -1)     # top pin names sit just inside the top edge
    draw_row(bot, "bottom", by1, -12, +1)  # bottom pin names just inside the bottom edge

    # side SWD header
    if board["side"]:
        nm, sp = board["side"]
        sx = bx1 + 24
        T(sx+30, by0-8, nm, 13, "#111", "bold")
        for j, s in enumerate(sp):
            yy = by0 + 18 + j*30
            C(sx, yy, 6, "#d9d9d9", "#999")
            T(sx+16, yy+4, s, 12, "#333", "normal", "start")

    # legend
    ly = H-46
    T(x0-12, ly-22, "Modules (tag colour):", 13, "#111", "bold", "start")
    items = [m for m in MODCOL if any(c[0]==m for c in board["conn"].values())]
    lx = x0-12
    for m in items:
        R(lx, ly-12, 16, 16, MODCOL[m], MODCOL[m], 0, 3)
        T(lx+22, ly+1, m, 12.5, "#333", "normal", "start")
        lx += 30 + 8*len(m) + 26
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="{Wd}" height="{H}">' + "".join(e) + '</svg>'
    tmp = out_png + ".svg"
    open(tmp, "w", encoding="utf-8").write(svg)
    pix = fitz.open(tmp)[0].get_pixmap(matrix=fitz.Matrix(scale, scale))
    pix.save(out_png)
    import os; os.remove(tmp)
    print(out_png, pix.width, "x", pix.height, os.path.getsize(out_png), "bytes")


if __name__ == "__main__":
    render(STM32,  "../STM32/images/pinout_stm32.png")
    render(CC3200, "../CC3200/images/pinout_cc3200.png")
