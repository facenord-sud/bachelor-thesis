#include <JsonParser.h>
#include <Servo.h>
#include <TinkerKit.h>

JsonParser<32> parser;
JsonHashTable root;
JsonHashTable jsonObject;
String jsonString;

int pinServo0 = O1;
int pinServo1 = O2;
int pinLinearPot0 = I0;
int pinLinearPot1 = I1;

boolean has_message = false;

unsigned long currentMillis;

Servo servo0;vap
Servo servo1;

TKPotentiometer pot0(pinLinearPot0);
TKPotentiometer pot1(pinLinearPot1);

// pour le pot0
int previousValuePot0;
// pour pot1
int previousValuePot1;
// pour pot0
int valueToStop0;
// pour pot1
int valueToStop1;
int error = 20;

String jsonToSend = "";

void setup()
{
  Serial.begin(9600);
  servo0.attach(pinServo0);
  servo1.attach(pinServo1);
  servo0.write(93);
  servo1.write(93);
}

void send_json() {
  jsonToSend = "";
  encodeContiniousServo(pinServo0, servo0.read());
  addCaret();
  encodeContiniousServo(pinServo1, servo1.read());
  addCaret();
  encodeLinearPot(pinLinearPot0, pot0.read(), previousValuePot0);
  addCaret();
  encodeLinearPot(pinLinearPot1, pot1.read(), previousValuePot1);
  Serial.println("{" + jsonToSend + "}");
}

void encodeContiniousServo(int pin, int speedServo) {
  jsonToSend += "\"";
  jsonToSend += pin;
  jsonToSend += "\":{\"speed\":";
  jsonToSend += speedServo;
  jsonToSend += "}";
}

void addCaret() {
  jsonToSend += ",";
}

void encodeLinearPot(int pin, int potValue, int previousValue) {
  jsonToSend += "\"";
  jsonToSend += pin;
  jsonToSend += "\":{\"position\":";
  jsonToSend += potValue;
  addCaret();
  jsonToSend += "\"oldPosition\":";
  jsonToSend += previousValue;
  jsonToSend += "}";  
}

char* intToCharArray(int value) {
  String s = String(value);
  char buf[100];
  s.toCharArray(buf, 100);
  return buf;
}

//pour ouvrir la porte à moitiée: {"14":{"value":500},"10":{"speed":0}}
void processDoor() {
  // pour stopper l'ouverture de la porte
  if (valueToStop0 <= pot0.read() + error && valueToStop0 >= pot0.read() - error) {
    servo0.write(93);
    valueToStop0 = -100;
  }
  // pour stopper le verrouillage de la porte
  if (valueToStop1 >= pot1.read() + error && valueToStop1 <= pot1.read() - error) {
    servo1.write(93);
    valueToStop1 = -100;
  } 
  // Empeche l'ouverture si la porte est verrouillée
  if(pot1.read() > 860 && servo0.read() != 93) {
    servo0.write(93);  
  }
  // Stop la fermeture quand la porte est fermée
  if(pot0.read() < 30 && servo0.read() > 93) {
    servo0.write(93);
  }
  // Stop l'ouverture quand la porte est ouverte
  if(pot0.read() > 970 && servo0.read() < 93) {
    servo0.write(93);
  }
  // Stop le verrouillage quand verrouillé
  if(pot1.read() < 30 && servo1.read() > 93) {
    servo1.write(93);
  }
  // Stop le déverrouillage quand déverrouillé
  if(pot1.read() > 860 && servo1.read() < 93) {
    servo1.write(93);
  }
}

void controlServo(Servo servo, int speedServo) {
  servo.write(speedServo);
}

void processLinearPot() {
// Quand envoyer le json au serveur
  if(
   (previousValuePot0 >= pot0.read() + error || previousValuePot0 <= pot0.read() - error) ||
   (previousValuePot1 >= pot1.read() + error || previousValuePot1 <= pot1.read() - error)  
  )  {
    send_json();
    previousValuePot0 = pot0.read();
    previousValuePot1 = pot1.read();
  }
}

void loop() {
  while (Serial.available() > 0) {
    jsonString = Serial.readString();
    char json[jsonString.length() + 1];
    jsonString.toCharArray(json, jsonString.length() + 1);
    root = parser.parseHashTable(json);
    // si JSON correcte
    if(root.success()) {
      //vitesse pour le servo O1
      jsonObject =  root.getHashTable(intToCharArray(pinServo0));
      if(jsonObject.success()) {
        controlServo(servo0, jsonObject.getLong("speed"));
      }
      //vitesse pour le servo O2
     jsonObject =  root.getHashTable(intToCharArray(pinServo1));
      if(jsonObject.success()) {
        controlServo(servo1, jsonObject.getLong("speed"));
      } 
      //Position à laquelle stoppper le moteur O1
      jsonObject = root.getHashTable(intToCharArray(pinLinearPot0));
    if (jsonObject.success()) {
      int stopPosition = jsonObject.getLong("position");
      if(stopPosition != 0) {
        valueToStop0 = stopPosition;
      }
    }
    // Position à laquelle stopper le moteur O2
    jsonObject = root.getHashTable(intToCharArray(pinLinearPot1));
    if (jsonObject.success()) {
      int stopPosition = jsonObject.getLong("position");
      if(stopPosition != 0) {
        valueToStop1 = stopPosition;
      }
    }
      has_message = true;
    }
  }
  processDoor();
  processLinearPot();
  has_message = false;
  delay(100);
}
