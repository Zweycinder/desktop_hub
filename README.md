# PC Command Hub

A little Arduino Uno desk gadget that sits next to your PC and shows live system
stats, a desk pet with moods, an audio visualizer, and a couple of games and
novelty modes — all on a 16x2 character LCD, navigated with a joystick.

```
┌────────────────┐
│ Happy pet face │   <- Pet mode (default home screen)
│ H 80  Hu 2 E 97│
└────────────────┘
```

## Features

- **Performance** — CPU/RAM/GPU load, temps, VRAM, ping/jitter/loss, network
  speed, and an auto-cycle page. Flip between sub-pages with left/right.
- **Visualizer** — a 16-band bar graph driven by your PC's actual audio
  output (WASAPI loopback + FFT), with 3 display styles (double-tap to cycle).
- **Dino runner** — jump/duck over scrolling obstacles, speeds up over time.
- **Desk Pet** — a face with 8 moods (Happy, Asleep, Hungry, Panicked,
  Excited/dancing, Bored, Laggy, Stuffed) driven by real PC conditions, plus
  two random "quirk" animations (smoking, coffee break). EEPROM-persisted
  across power cycles. Double-tap to feed it.
- **Hacking** (secret-ish) — a scrolling fake-hacking screensaver.
- **Wheel of Fortune** (secret-ish) — double-tap to spin, lands on 1 of 8 slots.
- **Coffee Flip** — a themed coin flip for "should I get another coffee".

An alert banner flashes over whatever's on screen if CPU/GPU temp or network
ping/loss crosses a threshold, regardless of which mode you're in.

## Hardware

| Part | Notes |
|---|---|
| Arduino Uno (or clone, ATmega328P) | |
| 16x2 character LCD with PCF8574 I2C backpack | Sold as "1602 I2C LCD" / "LCM1602 IIC". Default I2C address `0x27`. |
| Joystick module (HW-504 or similar) | X/Y analog + push switch |
| DS1302 RTC module | Needs a CR2032 battery. See [Known issues](#known-issues) below — the agent self-corrects drift, so this is not critical. |
| Mini breadboard / Sensor Shield V5 | For power distribution; optional |

No stepper motor / physical needle in this build — an earlier revision used
one, but it was removed in favor of keeping things simpler and more reliable.

### Wiring

| Function | Pin |
|---|---|
| LCD SDA | A4 |
| LCD SCL | A5 |
| Joystick X (VRx) | A0 |
| Joystick Y (VRy) | A1 |
| Joystick switch (SW) | D7 |
| RTC CLK | D6 |
| RTC DAT | A2 |
| RTC RST | A3 |

D0/D1 are reserved for USB serial — never wire anything there. D2-D5, D8-D13
are currently unused and free for future expansion.

**Power:** everything runs off the Uno's 5V rail during prototyping. Make
sure the LCD, joystick, and RTC all share a common GND with the Uno.

## Controls

- **Tap** the joystick switch: opens the mode menu (or confirms the
  highlighted selection if the menu's already open).
- **Up/Down** while the menu is open: scroll between modes.
- **Double-tap**: while the menu is open, cancels without picking anything;
  while inside a mode, it's that mode's contextual action (feed the pet,
  reset the network peak, cycle the visualizer style, flip a coin, spin the
  wheel, etc).
- **Left/Right** inside a mode: page-flip where applicable (Performance's
  sub-pages, the pet's face/status screens).
- **Up/Down** inside a mode: mode-specific (e.g. Dino's jump/duck).

## Setup

### 1. Firmware (Arduino IDE)

1. Open `firmware/firmware.ino` in the Arduino IDE (the sketch folder name
   must match the `.ino` filename, which it already does).
2. Install these libraries via **Library Manager** (Tools → Manage Libraries):
   - `LiquidCrystal I2C`
   - `Rtc by Makuna`
   - (`Wire` and `EEPROM` are built in, no install needed)
3. Select **Arduino Uno** as the board, pick the right COM port, and upload.

### 2. PC agent (Windows, Python 3.10+)

The agent reads your PC's sensors and feeds them to the Uno over serial.

```powershell
cd pc-agent
pip install -r requirements.txt
python agent.py
```

It auto-detects the Arduino's COM port. Edit `config.yaml` if you want to
pin a specific port, change ping targets, or adjust update rates.

**CPU/SSD temperature** requires
[LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor)
running with its Options → **Remote Web Server** enabled (default port 8085).
Without it, temps just show as 0 — everything else still works.

**GPU stats** need an NVIDIA GPU (uses `pynvml`/`nvidia-ml-py`). If you have
a different GPU, GPU load/temp/VRAM will stay at 0; nothing else is affected.

#### Run it automatically at login

Create a shortcut in your Startup folder
(`shell:startup` in the Windows Explorer address bar) pointing at:

```
Target:    C:\Path\To\pythonw.exe "C:\Path\To\pc-agent\agent.py"
Start in:  C:\Path\To\pc-agent
```

Using `pythonw.exe` (not `python.exe`) avoids a console window popping up.
The agent logs to `pc-agent/agent.log` and automatically retries/restarts on
most errors, so it's reasonably safe to just leave running.

## Known issues

- **RTC drift**: the DS1302's battery backup has proven unreliable in testing
  (loses the correct time after a power loss or sleep). The agent works
  around this by re-pushing the PC's clock to the Uno every 60 seconds while
  connected, so the RTC self-corrects — but if the agent isn't running, the
  clock (and the pet's night/Asleep trigger) can drift until it reconnects.
- **Excited/dancing mood** needs ~3 seconds of continuous audio before it
  triggers (to avoid reacting to notification dings), and ~2 seconds of
  quiet before it turns back off.
- Memory on the Uno is tight (32KB flash / 2KB RAM) — adding new modes may
  require trimming something else to fit.

## Project layout

```
firmware/        Arduino sketch (one mode per .cpp/.h pair)
pc-agent/        Python agent (collectors/ for system, network, audio)
```

## License

MIT — see [LICENSE](LICENSE).
