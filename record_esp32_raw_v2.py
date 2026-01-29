import serial

# Config
port = '/dev/ttyUSB0'  # Adjust as needed
baud = 2000000
duration_sec = 10
sample_rate = 44100
channels = 2
bytes_per_sample = 2 # We are enconding samples with int16_t, 16 bits
total_bytes = sample_rate * duration_sec * channels * bytes_per_sample

filename = "esp32_recording.raw"

ser = serial.Serial(port, baud)
print("Waiting for ESP32...")

# Wait for the ready signal
while True:
    line = ser.readline().decode(errors='ignore').strip()
    if "READY_TO_RECORD" in line:
        print("ESP32 is ready. Receiving audio...")
        break

with open(filename, "wb") as f:
    received = 0
    while received < total_bytes:
        chunk = ser.read(min(1024, total_bytes - received))
        f.write(chunk)
        received += len(chunk)
        print(f"{received}/{total_bytes} bytes", end='\r')

print(f"\nDone! Audio saved to: {filename}")
ser.close()
