#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "LUSUKU_HOME_4G";
const char* password = "lusuku2024";

// Server URL to send the data
const char* serverName = "http://192.168.1.141:8000/ultrasonic/sensor-data/";

// Define the pins for the ultrasonic sensor 1
#define TRIG1_PIN 4
#define ECHO1_PIN 2

// Define the pins for the ultrasonic sensor 2
#define TRIG2_PIN 32
#define ECHO2_PIN 33

long distance1;
long distance2;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");

  // Set pin modes for sensor 1
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);

  // Set pin modes for sensor 2
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
}

long measureDistance(int trigPin, int echoPin) {
  // Send a pulse to the TRIG pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the pulse from the ECHO pin
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance in centimeters
  long distance = duration / 58.2;
  return distance;
}

void sendData(long distance1, long distance2) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"distance1\": " + String(distance1) + ", \"distance2\": " + String(distance2) + "}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}

void loop() {
  // Measure distance for sensor 1
  distance1 = measureDistance(TRIG1_PIN, ECHO1_PIN);
  Serial.print("Distance 1: ");
  Serial.print(distance1);
  Serial.println(" cm");

  // Measure distance for sensor 2
  distance2 = measureDistance(TRIG2_PIN, ECHO2_PIN);
  Serial.print("Distance 2: ");
  Serial.print(distance2);
  Serial.println(" cm");

  // Send data to the Django server
  sendData(distance1, distance2);

  // Wait for 2 minutes before measuring again
  delay(2000);
}
