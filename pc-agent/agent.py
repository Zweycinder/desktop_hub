import logging
import sys
import time
import traceback
from pathlib import Path

import serial
import serial.tools.list_ports
import yaml

sys.path.insert(0, str(Path(__file__).parent))
from collectors import network, system
from collectors.audio import AudioVisualizer

MODE_VISUALIZER = 1

_handlers = [logging.FileHandler(Path(__file__).parent / "agent.log")]
if sys.stdout is not None:  # pythonw.exe / Task Scheduler have no real stdout
    _handlers.append(logging.StreamHandler(sys.stdout))
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s", handlers=_handlers)
log = logging.getLogger("agent")


def load_config():
    with open(Path(__file__).parent / "config.yaml") as f:
        return yaml.safe_load(f)


def find_arduino_port():
    for p in serial.tools.list_ports.comports():
        if "arduino" in (p.description or "").lower():
            return p.device
    return None


def open_serial(cfg):
    port = cfg["serial"]["port"]
    if port == "auto":
        port = find_arduino_port()
        if port is None:
            return None
    try:
        return serial.Serial(port, cfg["serial"]["baud"], timeout=0)
    except serial.SerialException:
        return None


def run():
    cfg = load_config()
    ping_stats = {t: network.PingStats(t, cfg["ping"]["window"]) for t in cfg["ping"]["targets"]}
    net_speed = network.NetSpeed()
    # Always running (not gated to Visualizer mode): Pet mode's Excited/dance
    # trigger needs to know "is music playing" regardless of what's on screen.
    # Only the heavy per-band 'A' hex frames stay gated to Visualizer itself.
    audio_vis = AudioVisualizer()
    audio_vis.start()
    current_mode = None
    rx_buf = ""

    ser = None
    last_heartbeat = 0.0
    last_system = 0.0
    last_network = 0.0
    last_audio = 0.0
    last_time_sync = 0.0

    log.info("PC agent starting. Waiting for Arduino...")
    while True:
        if ser is None:
            ser = open_serial(cfg)
            if ser is None:
                time.sleep(2)
                continue
            log.info(f"Connected on {ser.port}")

        try:
            now = time.monotonic()

            # Drain incoming lines (M,<modeId> / R) without blocking.
            waiting = ser.in_waiting
            if waiting:
                rx_buf += ser.read(waiting).decode("ascii", errors="ignore")
                while "\n" in rx_buf:
                    line, rx_buf = rx_buf.split("\n", 1)
                    line = line.strip()
                    if line.startswith("M,"):
                        current_mode = int(line[2:])
                        log.info(f"Mode changed to {current_mode}")
                    elif line == "R":
                        last_system = 0.0
                        last_network = 0.0
                    elif line:
                        log.info(f"[uno] {line}")  # any other firmware debug text

            if now - last_heartbeat >= 1.0 / cfg["update"]["heartbeat_hz"]:
                last_heartbeat = now
                ser.write(b"H\n")

            # The DS1302's battery backup has proven unreliable (drifts to a
            # wrong time after power loss/sleep), so rather than relying on
            # it to hold time correctly, periodically re-push the PC's clock
            # -- self-healing regardless of the RTC hardware's flakiness.
            if now - last_time_sync >= 60.0:
                last_time_sync = now
                ser.write(f"T,{int(time.time())}\n".encode("ascii"))

            # Y/N (system + network stats) are cheap, so these always run at
            # 1Hz regardless of mode -- the alert overlay and Pet mode's
            # Panicked/Laggy/Stuffed moods need fresh temp/ping data even
            # while a game or another mode is on screen, not just on the
            # System Pages. Only the heavier A (audio) / W (typing) streams
            # are gated to their own modes.
            if now - last_system >= 1.0 / cfg["update"]["system_hz"]:
                last_system = now
                cpu_pct, ram_pct = system.read_cpu_ram()
                gpu_pct, gpu_temp, vram_pct = system.read_gpu()
                cpu_temp, ssd_temp = system.read_lhm_temps(cfg["lhm"]["url"])
                audio_flag = 1 if audio_vis.is_music else 0
                line = f"Y,{cpu_pct},{ram_pct},{gpu_pct},{int(cpu_temp or 0)},{gpu_temp},{vram_pct},{int(ssd_temp or 0)},{audio_flag}\n"
                ser.write(line.encode("ascii"))

            if now - last_network >= 1.0 / cfg["update"]["network_hz"]:
                last_network = now
                for ps in ping_stats.values():
                    ps.sample()
                ping_ms, jitter_ms, loss_pct = next(iter(ping_stats.values())).current()
                dl_kbs, ul_kbs = net_speed.sample()
                line = f"N,{ping_ms},{jitter_ms},{loss_pct},{dl_kbs},{ul_kbs}\n"
                ser.write(line.encode("ascii"))

            if current_mode == MODE_VISUALIZER and now - last_audio >= 1.0 / cfg["update"]["audio_hz"]:
                last_audio = now
                ser.write(f"A,{audio_vis.hex_frame()}\n".encode("ascii"))

            time.sleep(0.02)

        except (serial.SerialException, OSError):
            log.warning("Arduino disconnected, retrying...")
            ser = None
            time.sleep(1)
        except Exception:
            # A collector hiccup (LHM unreachable, audio device hot-swap,
            # etc.) should never take the whole agent down -- log it and
            # keep going rather than silently dying.
            log.error("Unexpected error in main loop, continuing:\n" + traceback.format_exc())
            time.sleep(1)


if __name__ == "__main__":
    while True:
        try:
            run()
        except Exception:
            log.critical("Agent crashed, restarting in 5s:\n" + traceback.format_exc())
            time.sleep(5)
