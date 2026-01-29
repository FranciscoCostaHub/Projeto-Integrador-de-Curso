#include <driver/i2s.h>
#include <math.h>

// --- Serial UART Config ---
#define TX_PIN 17  // Connect to Pico GP5 for esp1, GP1 for esp2
#define RX_PIN -1  // Not used

// --- I2S Audio Config ---
#define I2S_WS 15
#define I2S_SD 12
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define bufferLen 64
int16_t sBuffer[bufferLen * 2];

const float referenceRMS = 200.0;
const float referenceDB = 94.0;
const float thresholdDB = 110.0;
const float micDistance = 0.11;
const float soundSpeed = 343.0;

const char* esp_id = "esp1";  // Change to "esp2" on second ESP

void i2s_install() {
  const i2s_config_t config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
  i2s_driver_install(I2S_PORT, &config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

float compute_rms(int16_t* data, int count) {
  float sum = 0;
  for (int i = 0; i < count; i++) sum += data[i] * data[i];
  return sqrt(sum / count);
}

float to_dB(float rms) {
  return referenceDB + 20.0 * log10(rms / referenceRMS);
}

float cross_correlation(int16_t* x, int16_t* y, int len, int lag) {
  float sum = 0;
  for (int i = 0; i < len - abs(lag); i++) {
    int xi = (lag >= 0) ? x[i + lag] : x[i];
    int yi = (lag >= 0) ? y[i] : y[i - lag];
    sum += xi * yi;
  }
  return sum;
}

void setup() {
  Serial.begin(115200);  // USB debug
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // TX to Pico

  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
}

void loop() {
  size_t bytesIn = 0;
  i2s_read(I2S_PORT, &sBuffer, sizeof(sBuffer), &bytesIn, portMAX_DELAY);

  if (bytesIn > 0) {
    int samples = bytesIn / (2 * sizeof(int16_t));
    int16_t left[samples], right[samples];
    for (int i = 0, j = 0; i < samples; i++) {
      left[i] = sBuffer[j++];
      right[i] = sBuffer[j++];
    }

    float rmsL = compute_rms(left, samples);
    float rmsR = compute_rms(right, samples);
    float dBL = to_dB(rmsL);
    float dBR = to_dB(rmsR);

    if (dBL >= thresholdDB && dBR >= thresholdDB) {
      Serial.println("Gunshot Detected!");

      int bestLag = 0;
      float bestCorr = -1e9;
      for (int lag = -50; lag <= 50; lag++) {
        float corr = cross_correlation(left, right, samples, lag);
        if (corr > bestCorr) {
          bestCorr = corr;
          bestLag = lag;
        }
      }

      float deltaT = bestLag / 44100.0;
      float pathDiff = deltaT * soundSpeed;
      float angleRad = asin(pathDiff / micDistance);
      float angleDeg = angleRad * 180.0 / PI;
      float avgDB = (dBL + dBR) / 2.0;

      char msg[64];
      snprintf(msg, sizeof(msg), "%s,%.1f,%.1f", esp_id, angleDeg, avgDB);
      Serial.print("Sending: ");
      Serial.println(msg);
      Serial1.println(msg);  // Send to Pico

      delay(2000);
    }
  }
}