import psutil
import pynvml
import requests

_nvml_ready = False


def _ensure_nvml():
    global _nvml_ready
    if not _nvml_ready:
        pynvml.nvmlInit()
        _nvml_ready = True


def _find_hw_node(node, image_substr):
    for hw in node.get("Children", []):
        if image_substr in hw.get("ImageURL", ""):
            return hw
    return None


def _find_sensor_value(node, text_substr=None):
    # Depth-first search for a numeric "xx.x °C" sensor value, optionally
    # matching text_substr in the sensor's own label.
    text = node.get("Text", "")
    value = node.get("Value", "")
    if "°C" in value and (text_substr is None or text_substr.lower() in text.lower()):
        try:
            return float(value.split()[0])
        except (ValueError, IndexError):
            pass
    for child in node.get("Children", []):
        result = _find_sensor_value(child, text_substr)
        if result is not None:
            return result
    return None


def read_lhm_temps(lhm_url):
    """Returns (cpu_temp_c, ssd_temp_c), either may be None if unavailable."""
    try:
        resp = requests.get(lhm_url, timeout=1.5)
        resp.raise_for_status()
        root = resp.json()
        machine = root.get("Children", [{}])[0]
    except (requests.RequestException, ValueError, IndexError):
        return None, None

    cpu_hw = _find_hw_node(machine, "cpu.png")
    cpu_temp = _find_sensor_value(cpu_hw, "Package") if cpu_hw else None

    ssd_hw = _find_hw_node(machine, "hdd.png")
    ssd_temp = _find_sensor_value(ssd_hw) if ssd_hw else None

    return cpu_temp, ssd_temp


def read_cpu_ram():
    cpu_pct = int(psutil.cpu_percent(interval=None))
    ram_pct = int(psutil.virtual_memory().percent)
    return cpu_pct, ram_pct


def read_gpu():
    """Returns (gpu_pct, gpu_temp_c, vram_pct)."""
    try:
        _ensure_nvml()
        handle = pynvml.nvmlDeviceGetHandleByIndex(0)
        util = pynvml.nvmlDeviceGetUtilizationRates(handle)
        temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
        mem = pynvml.nvmlDeviceGetMemoryInfo(handle)
        vram_pct = int(mem.used * 100 / mem.total)
        return int(util.gpu), int(temp), vram_pct
    except pynvml.NVMLError:
        return 0, 0, 0
