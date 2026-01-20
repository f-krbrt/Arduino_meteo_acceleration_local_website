#include <Arduino_LSM9DS1.h>
#include <Wire.h>
#include "DHT20.h"

const unsigned long SAMPLE_PERIOD_MS = 100;
const unsigned long DHT_PERIOD = 2000;
const float ACC_THRESHOLD = 1.2;
const float TEMP_THRESHOLD = 28.0;
const float M_last_seconds = 10;
const int n_shock_max = 10;

// Pour faire la pause apres avoir appuyé sur le bouton
const int time_break = 5000;
unsigned long start_break =0;

// pins
const int PINLED = A6;
const int PIN_BUZZER = 12;
const int PIN_BUTTON = A7;

bool recording = false;


unsigned long lastSampleTime_acc = 0;
unsigned long lastDHT = 0;
int shock_count = 0;
unsigned long lasTimeAlarmOn = 0;

float lastTemperature = 0;
float lastHumidity = 0;

String commandBuffer = "";
bool alarm = false;

DHT20 dht;

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);




  while (!Serial) {
  }
  if (!IMU.begin()) {
    Serial.println("IMU init failed");
    while (1) {
    }
  }
  Serial.println("IMU ready. Waiting for START / STOP commands.");

  Wire.begin();
  IMU.begin();
  dht.begin();
}


// HandleSerialCommands reste inchangé pour l'instant je pense que c'est ok
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

void buttonCommand(unsigned long now) {
  if (digitalRead(PIN_BUTTON) == LOW) {
    shock_count = 0;
    alarm = false;
    start_break = now;
  }
}

/*
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
*/

void sendData(unsigned long now) {

  Serial.print("t=");
  Serial.print(now);
  Serial.print(", T=");
  Serial.print(lastTemperature);
  Serial.print(", H=");
  Serial.print(lastHumidity);
  Serial.print(", shock_count=");
  Serial.print(shock_count);
  Serial.print(", alarm=");
  Serial.println(alarm);
}





void check_over_speed() {
  //Serial.println("on est dans le check_over_speed");
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) {
    return;
  }




  IMU.readAcceleration(ax, ay, az);
  //Serial.println("voici la valeur de l'acc : ");
  //Serial.println(sqrt(ax * ax + ay * ay + az * az));
  if (sqrt(ax * ax + ay * ay + az * az) > ACC_THRESHOLD) {
    shock_count++;
    //Serial.println("shock count :");
    //Serial.print(shock_count);
  }
}

void check_temperature() {
  dht.read();
  lastTemperature = dht.getTemperature();
  lastHumidity = dht.getHumidity();
}




void setAlarmOn(unsigned long now) {
  digitalWrite(PINLED, HIGH);

  if (now - lasTimeAlarmOn >= 350){
    tone(PIN_BUZZER, 1000, 50);
    lasTimeAlarmOn = now;
  }

  //if ((now / 500) % 2) tone(PIN_BUZZER, 1000, 50);
}

void setAlarmOff() {
  digitalWrite(PINLED, LOW);
  //le buzzer s'arrete tout seul
}






void loop() {
  unsigned long now = millis();
  handleSerialCommands();
  buttonCommand(now);

  if (!recording) {
    return;
  }

  //Check Over Speed
  if (now - lastSampleTime_acc >= SAMPLE_PERIOD_MS) {
    lastSampleTime_acc = now;
    check_over_speed();
  }
  if (shock_count >= n_shock_max) {
    alarm = true;
  }

  //Check temperature
  if (now - lastDHT > DHT_PERIOD) {
    lastDHT = now;
    check_temperature();
    sendData(now);
  }

  if (lastTemperature > TEMP_THRESHOLD) {
    alarm = true;
  }

  if (lastTemperature < TEMP_THRESHOLD && shock_count < n_shock_max) {
    alarm = false;
  }

  //Activation de l'alarme (ou désactivation)
  if (alarm == true && (now - start_break)>time_break) {
    setAlarmOn(now);
  } else {
    setAlarmOff();
  }
}
