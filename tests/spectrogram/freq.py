import pyaudio
import numpy as np
import matplotlib.pyplot as plt

# Configuration
RATE = 44100           # Sample rate (Hz)
CHUNK = 4096           # Small chunk size for fast updates (min reasonable for FFT)
CHANNELS = 1
FORMAT = pyaudio.paInt16
MIN_FREQ = 10000       # Only analyze frequencies >10 kHz
SHOW_PLOT = True

def main():
    # Initialize PyAudio
    p = pyaudio.PyAudio()

    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    print(f"Listening... analyzing frequencies above {MIN_FREQ} Hz (Ctrl+C to stop)")

    if SHOW_PLOT:
        plt.ion()
        fig, ax = plt.subplots()

    try:
        while True:
            # Read audio data
            data = stream.read(CHUNK, exception_on_overflow=False)
            audio_data = np.frombuffer(data, dtype=np.int16).astype(np.float32)
            audio_data /= 32768.0  # normalize to [-1, 1]

            # Apply Hanning window
            windowed = audio_data * np.hanning(len(audio_data))

            # FFT
            fft_data = np.fft.rfft(windowed)
            freqs = np.fft.rfftfreq(len(windowed), 1.0 / RATE)
            magnitude = np.abs(fft_data)

            # Filter for frequencies above 10 kHz
            mask = freqs > MIN_FREQ
            if not np.any(mask):
                continue

            high_freqs = freqs[mask]
            high_magnitude = magnitude[mask]

            # Find peak above 10 kHz
            peak_idx = np.argmax(high_magnitude)
            peak_freq = high_freqs[peak_idx]
            peak_amp = high_magnitude[peak_idx]

            print(f"Peak frequency (>10kHz): {peak_freq:.1f} Hz, amplitude={peak_amp:.2f}")

            if SHOW_PLOT:
                ax.cla()
                ax.plot(high_freqs, high_magnitude)
                ax.set_xlim(MIN_FREQ, RATE / 2)
                ax.set_xlabel("Frequency (Hz)")
                ax.set_ylabel("Magnitude")
                ax.set_title(f"High-Freq Spectrum — Peak: {peak_freq:.1f} Hz")
                plt.pause(0.001)

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        stream.stop_stream()
        stream.close()
        p.terminate()
        if SHOW_PLOT:
            plt.ioff()
            plt.show()

if __name__ == "__main__":
    main()
