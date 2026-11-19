#include <Wire.h>
#include <KiwisIoT.h>

// Replace with your Wi-Fi credentials and KiwisIoT Topic ID
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";
const char* topic = "YOUR_KIWISIOT_TOPIC";

// Initialize the KiwisIoT client
KiwisIoT kiwisiot(ssid, pass, topic);

// MAX30205 I2C address (A0=A1=A2=GND) and temperature register
const uint8_t MAX30205_ADDR = 0x48;
const uint8_t MAX30205_TEMP_REG = 0x00;

// Fever alert threshold in degrees Celsius
const float FEVER_THRESHOLD_C = 37.5;

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 5000;

// Read 16-bit temperature from MAX30205 and convert to Celsius
float readBodyTemperature() {
  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(MAX30205_TEMP_REG);
  if (Wire.endTransmission(false) != 0) {
    return -999.0; // Return error sentinel if sensor is unreachable
  }

  Wire.requestFrom(MAX30205_ADDR, (uint8_t)2);
  if (Wire.available() < 2) {
    return -999.0;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  // Combine bytes into a signed 16-bit integer (0.00390625 C per LSB)
  int16_t rawTemp = (int16_t)((msb << 8) | lsb);
  return rawTemp * 0.00390625;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Uses D4 (SDA) and D5 (SCL) on XIAO ESP32C3

  // Start Wi-Fi and connect to the KiwisIoT MQTT broker
  kiwisiot.begin();
}

void loop() {
  // Maintain MQTT connection and process background tasks
  kiwisiot.run();

  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;

    float tempC = readBodyTemperature();
    if (tempC > -100.0) {
      int alertStatus = (tempC >= FEVER_THRESHOLD_C) ? 1 : 0;

      Serial.print("Skin Temperature (C): ");
      Serial.print(tempC, 2);
      Serial.print(" | Status: ");
      Serial.println(alertStatus == 1 ? "ELEVATED" : "NORMAL");

      // Publish temperature to Channel 0 and alert flag to Channel 1
      kiwisiot.send("0", tempC);
      kiwisiot.send("1", alertStatus);
    } else {
      Serial.println("MAX30205 read error. Check I2C wiring.");
    }
  }
}
