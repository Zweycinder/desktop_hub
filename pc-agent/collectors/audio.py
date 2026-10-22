import threading
import time

import numpy as np
import pyaudiowpatch as pyaudio

CHUNK = 1024
NUM_BANDS = 16
MUSIC_SUSTAIN_S = 3.0   # must be active this long before counting as "music"
MUSIC_RELEASE_S = 2.0   # must be quiet this long before dropping back off


class AudioVisualizer:
    """WASAPI loopback capture of the default output device, FFT'd into 16
    log-spaced bands normalized 0-15 with a slow-decaying per-band max so
    levels adapt to whatever's playing. Runs its own thread; callers just
    read .levels (a list of 16 ints)."""

    def __init__(self):
        self.levels = [0] * NUM_BANDS
        self.is_active = False  # raw "is there sound right now" (per-frame, for the bar graph)
        self.is_music = False   # debounced: sustained activity, for Pet mode's Excited/dance trigger
        self._active_since = None
        self._quiet_since = None
        self._band_max = [1.0] * NUM_BANDS
        self._smoothed = [0.0] * NUM_BANDS
        self._running = False
        self._thread = None
        self._pa = None
        self._stream = None

    def start(self):
        if self._running:
            return
        self._pa = pyaudio.PyAudio()
        device = self._find_loopback_device()
        if device is None:
            self._pa.terminate()
            self._pa = None
            return
        self._channels = device["maxInputChannels"]
        self._rate = int(device["defaultSampleRate"])
        self._stream = self._pa.open(
            format=pyaudio.paInt16, channels=self._channels, rate=self._rate,
            frames_per_buffer=CHUNK, input=True, input_device_index=device["index"],
        )
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if self._thread:
            self._thread.join(timeout=1)
        if self._stream:
            self._stream.close()
        if self._pa:
            self._pa.terminate()

    def _find_loopback_device(self):
        try:
            wasapi_info = self._pa.get_host_api_info_by_type(pyaudio.paWASAPI)
        except OSError:
            return None
        default_speakers = self._pa.get_device_info_by_index(wasapi_info["defaultOutputDevice"])
        if default_speakers.get("isLoopbackDevice"):
            return default_speakers
        for loopback in self._pa.get_loopback_device_info_generator():
            if default_speakers["name"] in loopback["name"]:
                return loopback
        return None

    def _run(self):
        band_edges = np.logspace(np.log10(30), np.log10(self._rate / 2), NUM_BANDS + 1)
        freqs = np.fft.rfftfreq(CHUNK, d=1.0 / self._rate)

        while self._running:
            try:
                raw = self._stream.read(CHUNK, exception_on_overflow=False)
            except OSError:
                continue
            samples = np.frombuffer(raw, dtype=np.int16)
            if self._channels > 1:
                samples = samples.reshape(-1, self._channels).mean(axis=1)
            samples = samples.astype(np.float32) / 32768.0

            spectrum = np.abs(np.fft.rfft(samples))

            # Global silence gate on overall signal level, not just each
            # band's own decaying max -- per-band ratios are inherently
            # unstable near zero (a handful of noisy FFT bins in a sparse
            # band can look like a huge ratio even though nothing is playing).
            total_mag = float(spectrum.mean())
            SILENCE_MAG = 0.02
            is_silent = total_mag < SILENCE_MAG
            self.is_active = not is_silent

            # Debounce into a "music" verdict: a notification ding or other
            # brief blip shouldn't make the pet think a song is playing, so
            # require sustained activity before flipping on, and sustained
            # quiet before flipping back off.
            now = time.monotonic()
            if self.is_active:
                self._quiet_since = None
                if self._active_since is None:
                    self._active_since = now
                elif not self.is_music and now - self._active_since >= MUSIC_SUSTAIN_S:
                    self.is_music = True
            else:
                self._active_since = None
                if self._quiet_since is None:
                    self._quiet_since = now
                elif self.is_music and now - self._quiet_since >= MUSIC_RELEASE_S:
                    self.is_music = False

            for i in range(NUM_BANDS):
                mask = (freqs >= band_edges[i]) & (freqs < band_edges[i + 1])
                mag = float(spectrum[mask].mean()) if mask.any() else 0.0

                self._band_max[i] = max(self._band_max[i] * 0.995, mag, SILENCE_MAG)
                raw_level = 0.0 if is_silent else mag / self._band_max[i] * 15

                # VU-meter-style smoothing: snap up fast, settle down slowly,
                # so consecutive FFT frames (~23ms each) don't look like noise.
                if raw_level > self._smoothed[i]:
                    self._smoothed[i] += (raw_level - self._smoothed[i]) * 0.6
                else:
                    self._smoothed[i] += (raw_level - self._smoothed[i]) * 0.15

                self.levels[i] = max(0, min(15, round(self._smoothed[i])))

    def hex_frame(self):
        return "".join(f"{v:X}" for v in self.levels)
