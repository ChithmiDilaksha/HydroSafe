#include <ESP8266WiFi.h>
#include <Servo.h>
#include "ESP8266Firebase.h"


#define WIFI_SSID "hydro"
#define WIFI_PASSWORD "uxey8844"  


#define FIREBASE_URL "https://hydrosafe-c77c3-default-rtdb.firebaseio.com/"

Firebase firebase(FIREBASE_URL);
Servo servo;


const int surfaceArea = 144;
const int totalVol = 2160;
const int trigPin = D5;
const int echoPin = D6;
const int sensorPin = D2;

volatile long pulse = 0;
unsigned long lastTime = 0;

long duration;
int distance;
int calcVol = 0;
int realVol = 0;
float flowRate = 0.0;

void ICACHE_RAM_ATTR increase() {
  pulse++;
}

void setup() {
  Serial.begin(9600);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());


  firebase.Connect_to_host();

  servo.attach(2);
  servo.write(0);

  // Initialize ultrasonic sensor and flow sensor
  pinMode(sensorPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Attach interrupt for flow sensor
  attachInterrupt(digitalPinToInterrupt(sensorPin), increase, RISING);
}

void loop() {
  // Control the servo based on Firebase spillGate1 value
  String path = "spillGates/spillGate1";
  int spillGate1State = firebase.getInt(path);

  if (spillGate1State == 1) {
    servo.write(90); // Open the gate (90°)
    Serial.println("Gate opened (90°)");
  } else if (spillGate1State == 0) {
    servo.write(0); // Close the gate (0°)
    Serial.println("Gate closed (0°)");
  } else {
    Serial.println("Unexpected value in spillGate1");
  }

  // Measure distance using ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  calcVol = distance * surfaceArea;
  realVol = totalVol - calcVol;

  // Calculate flow rate
  flowRate = 2.663 * pulse / 1000 * 1000 / 60;

  if (millis() - lastTime > 2000) {
    pulse = 0;
    lastTime = millis();
  }

  // Print values for debugging
  Serial.print("Volume: ");
  Serial.println(realVol);

  Serial.print("Flow Rate: ");
  Serial.print(flowRate);
  Serial.println(" cm³/s");

  firebase.setInt("volume", realVol);
  firebase.setFloat("waterflow", flowRate);

  delay(2000); // Wait before the next loop
}