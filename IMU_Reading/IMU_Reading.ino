

#include <Arduino_LSM9DS1.h>
#include <Wire.h>
#include "DHT20.h"

//Period d'echantillonage
const unsigned long SAMPLE_PERIOD_MS = 100;
const unsigned long DHT_PERIOD = 2000;

//Seuil de déclenchement de l'alarme
const float ACC_THRESHOLD = 1.2;
const float TEMP_THRESHOLD = 28.0;

// utilisé pour la fonction check_speed qui ne prend pas en compte un valeur de temps M impartie pour réaliser les n chocs.
const int n_shock_max = 10;
// Cependant d'autre variables fonctions plus bas sont implémentés pour réaliser la verification des N chocs en M secondes

// Pour faire la pause apres avoir appuyé sur le bouton
const int time_break = 5000;
unsigned long start_break =0;

//variable pour arreter le systeme si on double clique sur le bouton en moins en 0.5 secondes
unsigned long period_double_tap_btn = 2000;
unsigned long pause_drop_btn = 0;

// pins
const int PINLED = A6;
const int PIN_BUZZER = 12;
const int PIN_BUTTON = A7;

//Var essentiel
bool recording = false;
bool alarm = false;

// Variables de temps, utiliser pour faire un echantillons en respectant la période donnée (ex : now - lastSampleTime_acc < periode donnée)
unsigned long lastSampleTime_acc = 0;
unsigned long lastDHT = 0;

//compte le nombre de chocs
int shock_count = 0;

// variable utilisé pour faire bipper l'alarme sur une periode donnée
unsigned long lasTimeAlarmOn = 0;

//derniere donnée de température et humidité enregistré
float lastTemperature = 0;
float lastHumidity = 0;

String commandBuffer = "";

DHT20 dht;


///// Donnees pour la liste FIFO/////

//Nombre de chocs (n=5) maximum en M = 10 secondes
const int n_chocs = 5;
const int m_secs = 10000;


float liste_chocs[n_chocs];
int head = 0;
int tail = 0;




//-------------Fonctions-----------------
//---------------------------------------

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
    
    /*
    1 clic sur le bouton -> l'alarme se coupe pendant quelques secondes
    2 clics sur le bouton -> Le système s'éteint

    Le test "now - start_break < period_double_tap_btn" permet de savoir si on a déjà appuyé sur le bouton quelques 2 secondes (period_double_tap_btn secondes) auparavant.
    Le second appuie est alors interprété comme l'action d'arreter le système  

    Le test "now - pause_drop_btn > 300" est nécessaire. En effet sans ce test, durant un appuie sur le bouton on parcours plusieurs fois la boucle loop.
    et donc plusieurs fois cette fonction. Ce qui fait qu'un seul appui arrete directement le systeme.
    Ici on considère que l'appui dure moins de 300 ms, ce qui permet de séparer 2 appuie distincts.
    */

    if (now - start_break < period_double_tap_btn && now - pause_drop_btn > 300){
      recording = false;
      Serial.println("Le système va s'arrêter. ACK STOP !");
    }

    pause_drop_btn = now;
    shock_count = 0;
    alarm = false;
    start_break = now;
  }
}


void sendData(unsigned long now) {
  /*
  Cette fonction envoie les données sous forme de chaine de caratères
  */
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
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) {
    return;
  }
  IMU.readAcceleration(ax, ay, az);
  if (sqrt(ax * ax + ay * ay + az * az) > ACC_THRESHOLD) {
    shock_count++;
  }
}

void check_temperature() {
  /*
  Cette fonction récupère la température et l'humidité mesuré par le capteur dht
  */
  dht.read();
  lastTemperature = dht.getTemperature();
  lastHumidity = dht.getHumidity();
}




void setAlarmOn(unsigned long now) {
  /*
  Cette fonction active l'alarme (un bip et une LED)
  */
  digitalWrite(PINLED, HIGH);

  if (now - lasTimeAlarmOn >= 350){
    tone(PIN_BUZZER, 1000, 50);
    lasTimeAlarmOn = now;
  }
}

void setAlarmOff() {
  digitalWrite(PINLED, LOW);
  //le buzzer s'arrete tout seul
}

//-----------------------------------------
//------------Liste FIFO Fonctions---------
//-----------------------------------------

void push(unsigned long now) {
  /*
  Cette fonction ajoute la dernière valeur d'accélération (seulement si l'acceleration total est supérieur à ACC_THRESHOLD) au tableau liste_choc
  Elle augmente également le compteur de 1 si le tableau n'est pas rempli
  A noter que le tableau est une boucle
  */
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) {
    return;
  }
  IMU.readAcceleration(ax, ay, az);
  if (sqrt(ax * ax + ay * ay + az * az) > ACC_THRESHOLD) {

    liste_chocs[head] = now;
    head = (head + 1) % n_chocs;
 
    if (shock_count < n_chocs) {
      shock_count ++;
    } else {
      tail = (tail + 1) % n_chocs;
    }
  }
}

float get_tail(){
  /*
  Cette fonction récupère la queue de la liste (donc l'élement le plus ancien (et toujours présent évidement) entré dans le tableau)
  */
  float val_time = liste_chocs[tail];
  return val_time;
}

void delete_queue() {
  /*
  Cette fonction supprime la queue de la liste (donc l'élement le plus ancien (et toujours présent évidement) entré dans le tableau)
  En réalité l'élément n'est pas supprimé, on incrémente juste l'indice de "queue" pour rendre invisible l'élement précédent
  Cette fonction décrémente aussi le counter de chocs de un
  */
  if (shock_count == 0) return;
  tail = (tail + 1) % n_chocs;
  shock_count --;
}





//-------------------------------------------
//--------------------Loop--------------------
//--------------------------------------------


void loop() {
  unsigned long now = millis();
  handleSerialCommands();
  buttonCommand(now);

  if (!recording) {
    return;
  }

  //Check N chocs en M secondes
  if (now - lastSampleTime_acc >= SAMPLE_PERIOD_MS ){
    if(now - get_tail() > m_secs){
      delete_queue();
    }
    lastSampleTime_acc = now;
    push(now);
  }


  //Check temperature
  if (now - lastDHT > DHT_PERIOD) {
    lastDHT = now;
    check_temperature();
    sendData(now);
  }

  // Test dépassement de seuil de chocs ou de température
  if (now - get_tail() < m_secs && shock_count >= n_chocs){
    alarm = true;
  }
  if (lastTemperature > TEMP_THRESHOLD) {
    alarm = true;
  }


  //Sinon on remet l'alarm à False
  if (lastTemperature < TEMP_THRESHOLD && shock_count < n_chocs) {
    alarm = false;
  }

  //Activation de l'alarme (ou désactivation)
  if (alarm == true && (now - start_break)>time_break) {
    setAlarmOn(now);
  } else {
    setAlarmOff();
  }
}
