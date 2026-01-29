/*
  Sample sound from I2S microphone, display on Serial Plotter

  I2S MIC: https://dl.sipeed.com/MAIX/HDK/Sipeed-I2S_MIC/ https://mauser.pt/catalog/product_info.php?products_id=095-1985
  This microphone records 24-bit audio samples

  Based on the code from:
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include I2S driver
#include <driver/i2s.h>

// Connections to INMP441 I2S microphone
#define I2S_WS 15
#define I2S_DA 12
#define I2S_SCK 33

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

// Audio configuration
#define SAMPLE_RATE     44100
#define BITS_PER_SAMPLE 16
#define CHANNELS        2

// Define input buffer length
#define bufferLen 512
int16_t stereo_buffer[bufferLen]; // The microphone gives a 24 bit output, but we will treat it as 16 bit and avoid noise and serial bus constraints, but lowering the dynamic range.

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(BITS_PER_SAMPLE),
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_DA
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {

  // Set up Serial Monitor
  Serial.begin(2000000);
  Serial.println(" ");

  delay(1000);

  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  Serial.println("READY_TO_RECORD"); // Signal for the python script that it can start recording .raw data
  delay(2000); // Allow PC to get ready

}

void loop() {

  // This variable is used to toggle between pritinting the wave forms for debugging and sending the data over serial to be recorded in raw form.
  bool streaming = false;

  if (streaming) {

    // Send continuous byte stream
    while (true) {

      size_t bytesIn;

      // Get I2S data and place in data buffer
      i2s_read(I2S_PORT, stereo_buffer, sizeof(stereo_buffer), &bytesIn, portMAX_DELAY);
      if (bytesIn > 0) {
        Serial.write((uint8_t*)stereo_buffer, bytesIn);
      } 

    }

  } else {

    // Get I2S data and place in data buffer
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, stereo_buffer, sizeof(stereo_buffer), &bytesIn, portMAX_DELAY);

    // Read I2S data buffer
    int samples_read = bytesIn / sizeof(int16_t);
    int stereoFrames = samples_read / 2;
    if (samples_read > 0) {
      float meanLeft = 0;
      float meanRight = 0;
      for (int i = 0; i < samples_read; i+=2) {
        meanLeft += (stereo_buffer[i]);
        meanRight += (stereo_buffer[i + 1]);
      }
    

      // Average the data reading
      meanLeft /= stereoFrames;
      meanRight /= stereoFrames;

      // False print statements to "lock range" on serial plotter display
      // Change rangelimit value to adjust "sensitivity"
      int rangelimit = 1000;
      Serial.print(rangelimit * -1);
      Serial.print(" ");
      Serial.print(rangelimit);
      Serial.print(" ");

      // Print to serial plotter
      Serial.print(meanLeft);
      Serial.print(" ");
      Serial.println(meanRight);

    }

  }

}
