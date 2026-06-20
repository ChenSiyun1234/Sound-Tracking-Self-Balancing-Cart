#!/usr/bin/env python3
"""
BalanceBot Remote Controller
============================================================================
A small desktop app that drives the self-balancing two-wheel robot over its
HC-05 Bluetooth link. After the HC-05 is paired, Windows exposes it as a
serial COM port; this app sends single-character control commands
(F / B / L / R / S + a speed digit) that the robot's STM32 firmware parses.

This is the companion "remote" to the embedded firmware — it demonstrates the
software side (GUI, keyboard input, serial/Bluetooth I/O, a clean command
protocol) of the same project.

Usage
-----
  python balancebot_controller.py                 # GUI; simulation mode if no port
  python balancebot_controller.py --port COM5     # connect to the paired HC-05 port
  python balancebot_controller.py --selftest      # headless logic test (no GUI/HW)

Controls
--------
  W / Up    forward      A / Left   turn left
  S / Down  backward     D / Right  turn right
  Space     stop         + / -      speed up / down (0-9)
============================================================================
"""
import argparse

# --- Command protocol: one ASCII char per action (matches the firmware) ---
COMMANDS = {
    "forward": "F",
    "backward": "B",
    "left": "L",
    "right": "R",
    "stop": "S",
}


class BalanceBotLink:
    """Serial link to the HC-05, with a no-hardware simulation fallback."""

    def __init__(self, port=None, baud=9600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.sim = True          # True until a real port is open
        self.log = []            # commands sent (used in simulation / tests)
        if port:
            try:
                import serial   # pyserial
                self.ser = serial.Serial(port, baud, timeout=1)
                self.sim = False
            except Exception as exc:  # pyserial missing or port unavailable
                print(f"[sim] could not open {port} ({exc}); running in simulation mode")

    def send(self, cmd, speed=None):
        """Send a command char (optionally with a 0-9 speed digit). Returns bytes sent."""
        if cmd not in COMMANDS.values():
            raise ValueError(f"unknown command: {cmd!r}")
        if speed is not None and not (0 <= speed <= 9):
            raise ValueError("speed must be 0-9")
        payload = cmd if speed is None else f"{cmd}{speed}"
        data = payload.encode("ascii")
        if self.ser is not None:
            self.ser.write(data)
        else:
            self.log.append(payload)
        return data

    def close(self):
        try:
            self.send("S")          # safety: stop before disconnecting
        finally:
            if self.ser is not None:
                self.ser.close()


def run_gui(port=None):
    import tkinter as tk

    link = BalanceBotLink(port)
    state = {"speed": 5}

    root = tk.Tk()
    root.title("BalanceBot Remote")
    root.geometry("340x340")
    root.resizable(False, False)

    conn = "CONNECTED  " + port if not link.sim else "SIMULATION (no serial port)"
    tk.Label(root, text=conn, fg=("#1a7f37" if not link.sim else "#b06a00"),
             font=("Segoe UI", 10, "bold")).pack(pady=(10, 2))
    status = tk.StringVar(value="ready")
    tk.Label(root, textvariable=status, font=("Consolas", 11)).pack()

    def act(name):
        c = COMMANDS[name]
        link.send(c, state["speed"])
        status.set(f"{name.upper():8s} -> sent '{c}{state['speed']}'")

    def change_speed(delta):
        state["speed"] = max(0, min(9, state["speed"] + delta))
        status.set(f"speed = {state['speed']}")

    # --- directional pad ---
    grid = tk.Frame(root)
    grid.pack(pady=12)
    btn = dict(width=6, height=2, font=("Segoe UI", 11, "bold"))
    tk.Button(grid, text="▲\nW", command=lambda: act("forward"), **btn).grid(row=0, column=1, padx=4, pady=4)
    tk.Button(grid, text="◄ A", command=lambda: act("left"), **btn).grid(row=1, column=0, padx=4, pady=4)
    tk.Button(grid, text="STOP", fg="#b00020", command=lambda: act("stop"), **btn).grid(row=1, column=1, padx=4, pady=4)
    tk.Button(grid, text="D ►", command=lambda: act("right"), **btn).grid(row=1, column=2, padx=4, pady=4)
    tk.Button(grid, text="▼\nS", command=lambda: act("backward"), **btn).grid(row=2, column=1, padx=4, pady=4)

    spd = tk.Frame(root)
    spd.pack()
    tk.Button(spd, text="-  speed", command=lambda: change_speed(-1)).grid(row=0, column=0, padx=6)
    tk.Button(spd, text="speed  +", command=lambda: change_speed(+1)).grid(row=0, column=1, padx=6)

    # --- keyboard bindings ---
    keymap = {"w": "forward", "Up": "forward", "s": "backward", "Down": "backward",
              "a": "left", "Left": "left", "d": "right", "Right": "right",
              "space": "stop"}
    for key, name in keymap.items():
        root.bind(f"<{key}>", lambda e, n=name: act(n))
    root.bind("<plus>", lambda e: change_speed(+1))
    root.bind("<minus>", lambda e: change_speed(-1))
    root.bind("<KeyPress-equal>", lambda e: change_speed(+1))

    tk.Label(root, text="Keys: WASD / arrows · Space = stop · +/- speed",
             fg="#666", font=("Segoe UI", 8)).pack(side="bottom", pady=6)

    root.protocol("WM_DELETE_WINDOW", lambda: (link.close(), root.destroy()))
    root.mainloop()


def selftest():
    """Exercise the command protocol with no GUI or hardware."""
    link = BalanceBotLink(None)   # simulation mode
    assert link.sim is True
    for name, ch in COMMANDS.items():
        assert link.send(ch, 5) == f"{ch}5".encode("ascii")
    assert link.send("F") == b"F"
    try:
        link.send("Z")            # invalid command must raise
        raise AssertionError("expected ValueError for bad command")
    except ValueError:
        pass
    print("sent in order:", link.log)
    print("SELFTEST OK — command protocol works")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="BalanceBot Bluetooth remote controller")
    ap.add_argument("--port", help="serial COM port of the paired HC-05 (e.g. COM5)")
    ap.add_argument("--selftest", action="store_true", help="run headless logic test")
    args = ap.parse_args()
    if args.selftest:
        selftest()
    else:
        run_gui(args.port)
