#include <Arduino_LSM9DS1.h>

const unsigned long SAMPLE_PERIOD_MS = 100;  // 10 Hz

bool recording = false;  // set by START/STOP commands from PC

unsigned long lastSampleTime = 0;

String commandBuffer;  // to accumulate incoming serial commands

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // wait for Serial on Nano 33 BLE
  }

  if (!IMU.begin()) {
    Serial.println("IMU init failed");
    while (1) {
      // stop here if IMU not available
    }
  }

  Serial.println("IMU ready. Waiting for START / STOP commands.");
}

void handleSerialCommands() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        String cmd = commandBuffer;
        commandBuffer = "";

        cmd.trim();
        cmd.toUpperCase();

        if (cmd == "START") {
          recording = true;
          Serial.println("ACK:START");
        } else if (cmd == "STOP") {
          recording = false;
          Serial.println("ACK:STOP");
        } else {
          Serial.print("ERR:UNKNOWN_CMD ");
          Serial.println(cmd);
        }
      }
    } else {
      commandBuffer += c;
      if (commandBuffer.length() > 50) {
        commandBuffer = "";
      }
    }
  }
}

void sendIMUSample(unsigned long now) {
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) {
    return;
  }
  IMU.readAcceleration(ax, ay, az);
  Serial.print("t=");
  Serial.print(now);
  Serial.print(",ax=");
  Serial.print(ax, 4);
  Serial.print(",ay=");
  Serial.print(ay, 4);
  Serial.print(",az=");
  Serial.println(az, 4);
}

void 

void loop() {
  unsigned long now = millis();
  handleSerialCommands();
  char c = (char)Serial.read();
  if (!recording) {
    return;
  }
  // 10 Hz sampling
  if (now - lastSampleTime >= SAMPLE_PERIOD_MS) {
    lastSampleTime = now;
    sendIMUSample(now);
  }
}


















