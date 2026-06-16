#!/usr/bin/env python3
"""
gen_schematic.py - generate a full connection SCHEMATIC: the MCU drawn with all its used
pins (grouped on left/right), every peripheral module drawn as a component with its OWN pins,
signal wires routed pin-to-pin (colour = module), and power shown with rail flags (3V3/5V/GND)
fed from the power-hub. Original artwork (not a vendor datasheet). SVG -> PNG via PyMuPDF.

INPUT per board: mcu name + left_modules / right_modules, each:
    (title, [ (module_pin, mcu_pin), ... ] signal pins, [ (module_pin, rail), ... ] power pins )
Edit the board data and run `python gen_schematic.py`.
"""
import fitz

MODCOL = {
    "MPU-6050":"#2980b9","HC-05":"#8e44ad","HC-SR04":"#16a085",
    "TB6612":"#c0392b","Encoders":"#27ae60","OLED":"#e67e22","UART":"#7f8c8d",
}
RAILCOL = {"3V3":"#d4a017","5V":"#c0392b","GND":"#34495e","VM":"#922b21"}

STM32 = {
    "mcu":"STM32F103C8T6  (Blue Pill)",
    "mcu_note":["Cortex-M3 @ 72 MHz","balance loop @ 100 Hz","in the MPU data-ready ISR"],
    "hub":"TB6612 / MD220A  —  power hub:  battery 6-7.4 V  ->  5 V / 5 A  +  3.3 V / 500 mA",
    "left":[
        ("MPU-6050",[("SCL","PB10"),("SDA","PB11"),("INT","PA5")],[("VCC","3V3"),("GND","GND")]),
        ("HC-05",[("TXD","PA3"),("RXD","PA2")],[("VCC","5V"),("GND","GND")]),
        ("HC-SR04",[("TRIG","PB0"),("ECHO","PA12")],[("VCC","5V"),("GND","GND")]),
    ],
    "right":[
        ("TB6612",[("PWMA","PA8"),("PWMB","PA11"),("AIN1","PB12"),("AIN2","PB13"),
                   ("BIN1","PB14"),("BIN2","PB15"),("STBY","PA4")],[("VM","VM"),("VCC","5V"),("GND","GND")]),
        ("Encoders",[("E1A","PA15"),("E1B","PB3"),("E2A","PB4"),("E2B","PB5")],[("VCC","5V"),("GND","GND")]),
        ("OLED",[("SCL","PB8"),("SDA","PB9"),("VCC","PB7"),("GND","PB6")],[]),
    ],
    "subtitle":"MCU pins grouped per module; coloured wires = signals; flags (3V3/5V/GND) = power from the MD220A hub.",
}

CC3200 = {
    "mcu":"CC3200 LaunchPad",
    "mcu_note":["Cortex-M4 + Wi-Fi","Kalman fusion @ 200 Hz","Wi-Fi -> AWS IoT alerts"],
    "hub":"TB6612 driver   ·   board 3.3 V -> P1.1 (remove J13)",
    "left":[
        ("MPU-6050",[("SCL","PIN_01"),("SDA","PIN_02")],[("VCC","3V3"),("GND","GND")]),
        ("UART",[("TX","PIN_55"),("RX","PIN_57")],[]),
    ],
    "right":[
        ("TB6612",[("PWM","PIN_64"),("AIN1","PIN_63"),("AIN2","PIN_53"),
                   ("BIN1","PIN_50"),("BIN2","PIN_45"),("STBY","3V3")],[("VM","VM"),("GND","GND")]),
        ("OLED",[("SCK","PIN_05"),("MOSI","PIN_07"),("D/C","PIN_61"),("CS","PIN_62"),("RST","PIN_18")],[("VCC","3V3"),("GND","GND")]),
    ],
    "subtitle":"MCU pins grouped per module; coloured wires = signals; flags = power; Wi-Fi is on-chip.",
}

def render(b, out_png, scale=1.7):
    e=[]
    def R(x,y,w,h,fill,st="#333",sw=2,rx=6): e.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" fill="{fill}" stroke="{st}" stroke-width="{sw}"/>')
    def L(x1,y1,x2,y2,c,sw=2): e.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{c}" stroke-width="{sw}"/>')
    def Cc(cx,cy,r,fill,st="#555",sw=1): e.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r}" fill="{fill}" stroke="{st}" stroke-width="{sw}"/>')
    def T(x,y,s,sz=13,fill="#111",w="normal",a="start"): e.append(f'<text x="{x:.1f}" y="{y:.1f}" font-family="Helvetica,Arial,sans-serif" font-size="{sz}" fill="{fill}" font-weight="{w}" text-anchor="{a}">{s}</text>')

    W,H = 1820, 1180
    e.append(f'<rect x="0" y="0" width="{W}" height="{H}" fill="#ffffff"/>')
    T(W/2,42,b["mcu"]+"  —  Connection Schematic",24,"#111","bold","middle")
    T(W/2,66,b["subtitle"],14,"#666","normal","middle")

    # ---- layout the MCU pins per side (groups with gaps) ----
    def layout(mods, ytop, pitch=40, gap=30):
        ys, groups = [], []
        y=ytop
        for (name,sig,pwr) in mods:
            g=[]
            for (mp,mc) in sig:
                g.append((y,mp,mc,name)); y+=pitch
            groups.append((name,g,pwr)); y+=gap
        return groups, y-gap
    Ltop, Rtop = 150, 150
    Lg, Lend = layout(b["left"], Ltop)
    Rg, Rend = layout(b["right"], Rtop)

    mcx0,mcx1 = 700,1120
    mcy0 = 120; mcy1 = max(Lend,Rend)+30
    R(mcx0,mcy0,mcx1-mcx0,mcy1-mcy0,"#dceefb","#0b3d91",3,12)
    T((mcx0+mcx1)/2,mcy0+34,b["mcu"],18,"#0b3d91","bold","middle")
    for i,n in enumerate(b["mcu_note"]): T((mcx0+mcx1)/2,mcy0+58+i*20,n,12.5,"#33557a","normal","middle")

    mod_w=210
    def draw_side(groups, side):
        for (name,g,pwr) in groups:
            col=MODCOL[name]
            y0=g[0][0]-30; y1=g[-1][0]+30
            if side=="L":
                bx1=mcx0-160; bx0=bx1-mod_w; pinx=bx1; mcux=mcx0
            else:
                bx0=mcx1+160; bx1=bx0+mod_w; pinx=bx0; mcux=mcx1
            R(bx0,y0,mod_w,y1-y0,"#f7f9fb",col,2.5)
            T((bx0+bx1)/2,y0+22,name,15.5,col,"bold","middle")
            L(bx0+10,y0+30,bx1-10,y0+30,"#ddd",1)
            for (py,mp,mc,_) in g:
                # module pin stub + label
                Cc(pinx,py,4,col,col);
                if side=="L": T(pinx-12,py+4,mp,12.5,"#222","normal","end")
                else: T(pinx+12,py+4,mp,12.5,"#222","normal","start")
                # MCU pin stub + label
                Cc(mcux,py,4,"#0b3d91","#0b3d91")
                if side=="L": T(mcux+12,py+4,mc,12,"#0b3d91","bold","start")
                else: T(mcux-12,py+4,mc,12,"#0b3d91","bold","end")
                # wire
                L(pinx,py,mcux,py,col,2.4)
            # power flags at the bottom of the module box
            px=bx0+18
            for (mp,rail) in pwr:
                rc=RAILCOL.get(rail,"#777")
                T(px,y1-12,f"{mp}",11.5,"#555","normal","start")
                R(px+ (8*len(mp))+6, y1-24, 30,16, rc, rc,0,3)
                T(px+(8*len(mp))+21, y1-12, rail, 10.5, "#fff","bold","middle")
                px += (8*len(mp))+6+30+16
    draw_side(Lg,"L"); draw_side(Rg,"R")

    # ---- power hub strip ----
    hy=H-150
    R(60,hy,W-120,96,"#fdf3e7","#d68910",2,10)
    T(80,hy+26,"POWER",15,"#922b21","bold")
    T(150,hy+26,b["hub"],14,"#5a3a16")
    # rail bars
    rails=[("5V","#c0392b"),("3V3","#d4a017"),("GND","#34495e"),("VM (motor)","#922b21")]
    for i,(rn,rc) in enumerate(rails):
        ry=hy+50+ (i//2)*22; rx=160+(i%2)*360
        R(rx,ry-12,26,16,rc,rc,0,3); T(rx+13,ry,rn.split()[0],10,"#fff","bold","middle")
        T(rx+34,ry,rn,12.5,"#444","normal","start")
    T(W/2, hy+88, "every module's VCC/GND flag ties to these rails; single common star ground", 12.5,"#777","italic","middle")

    # legend
    ly=H-26
    T(80,ly,"signal wires colour-coded by module:",13,"#111","bold")
    lx=440
    for m in [x[0] for x in b["left"]+b["right"]]:
        R(lx,ly-12,15,15,MODCOL[m],MODCOL[m],0,3); T(lx+20,ly,m,12.5,"#333"); lx+=40+8*len(m)+20

    svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}">'+"".join(e)+'</svg>'
    open(out_png+".svg","w",encoding="utf-8").write(svg)
    pix=fitz.open(out_png+".svg")[0].get_pixmap(matrix=fitz.Matrix(scale,scale)); pix.save(out_png)
    import os; os.remove(out_png+".svg"); print(out_png,pix.width,"x",pix.height,os.path.getsize(out_png),"bytes")

if __name__=="__main__":
    render(STM32,"../STM32/images/schematic_stm32.png")
    render(CC3200,"../CC3200/images/schematic_cc3200.png")
