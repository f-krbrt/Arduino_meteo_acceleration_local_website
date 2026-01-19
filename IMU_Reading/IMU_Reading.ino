#include <Arduino_LSM9DS1.h>
#include <Arduino_Lib_DHT20.h>

//Constantes
const unsigned long SAMPLE_PERIOD_MS = 100;  // 10 Hz acc
const unsigned long DHT_PERIOD = 2000; //temp
const float ACC_THRESHOLD = 1.5;
const float TEMP_THRESHOLD = 30.0;
const float M_last_seconds = 10;

//Pins
const int PIN_LED = 13;
const int PIN_BUZZER = 2;
const int PIN_BUTTON = 3;

bool recording = false;  // set by START/STOP commands from PC

//Var
unsigned long lastSampleTime_acc = 0;
unsigned long lastDHT = 0;
int shock_count = 0;

float lastTemperature = 0;
float lastHumidity = 0;

String commandBuffer = "";
bool alarm = false;

DHT20 dht;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  IMU.begin();
  dht.begin();

/*
  while (!Serial) {
  }
  if (!IMU.begin()) {
    Serial.println("IMU init failed");
    while (1) {
    }
  }
  Serial.println("IMU ready. Waiting for START / STOP commands.");
*/
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

void buttonCommand() {
  if ((digitalRead(PIN_BUTTON) == LOW) && alarm == true) {
    shock_count = 0;
    alarm = false;
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

void sendData(unsigned long now){

  Serial.print("t=");
  Serial.print(now);
  Serial.print(", T=");
  Serial.print(lastTemperature);
  Serial.print(", H=");
  Serial.print(lastHumidity);
  Serial.print(", shock_count=");
  Serial.print(shock_count);
  Serial.print(", alarm");
  Serial.print(alarm);

}

void check_over_speed(){
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) {
    return;
  }
  IMU.readAcceleration(ax, ay, az);
  if (sqrt(ax*ax + ay*ay + az*az)> ACC_THRESHOLD ) {
    shock_count++;
  }
}

void check_temperature(){
  dht.read();
  lastTemperature = dht.getTemperature();
  lastHumidity = dht.getHumidity();
}


void setAlarmOn(unsigned long now){
  digitalWrite(PIN_LED, (now/200)%2);
  if ((now / 500) % 2) tone(PIN_BUZZER, 1000, 50);
}




void loop() {
  unsigned long now = millis();
  handleSerialCommands();
  buttonCommand();
/*
  char c = (char)Serial.read();
  if (!recording) {
    return;
  }
*/

  //Check Over Speed
  if (now - lastSampleTime_acc >= SAMPLE_PERIOD_MS) {
    lastSampleTime_acc = now;
    check_over_speed();
    //sendIMUSample(now);
  }
  if (shock_count >= 10) {
    alarm = true;
  }

  //Check temperature
  if (now - lastDHT > DHT_PERIOD){
    lastDHT = now;
    check_temperature();
  }

  if (lastTemperature > TEMP_THRESHOLD) {
    alarm = true;
  }

  //Activation de l'alarme (ou désactivation)
  if (alarm == true){
    setAlarmOn(now);
  } else {
    digitalWrite(PIN_LED, LOW);
  }

  if (now - lastDHT > DHT_PERIOD){
    sendData(now);
  }
  
  
}


















