#!/bin/bash

# Call the python script to record .raw data
python record_esp32_raw_v2.py 

# Input raw file (32-bit stereo little-endian)
INPUT_FILE="esp32_recording.raw"

# Output file names
OUTPUT_MP3="output_audio.mp3"

# Audio parameters
SAMPLE_RATE=44100
CHANNELS=2
FORMAT=s16le  # 32-bit signed little endian

# Convert to MP3
ffmpeg -f $FORMAT -ar $SAMPLE_RATE -ac $CHANNELS -i "$INPUT_FILE" "$OUTPUT_MP3"

echo "Conversion complete:"
echo "  MP3: $OUTPUT_MP3"

