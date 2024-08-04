#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "LUSUKU_HOME_4G";
const char* password = "lusuku2024";

// Django server URL
const char* serverUrl = "http://192.168.1.140:8000/api/upload/";

#define CAMERA_MODEL_AI_THINKER

// Define the pins for the ultrasonic sensor
const int trigPin = 12;
const int echoPin = 15; // Changed from 15 to avoid conflict

// Define the servo pin and initial position
#define SERVO_PIN 14 // Changed from 15 to 16
Servo myServo;

// Declare variables
long duration; // Duration of the pulse
float distance; // Distance measurement in cm
const float measurement = 13.0; // Threshold distance to trigger photo capture (in cm)

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  #define LED_FLASH_GPIO_NUM  4 // Define the LED flash GPIO pin
#else
  #error "Camera model not selected"
#endif

#define FILE_PHOTO_PATH "/photo.jpg"

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Attempt to mount LittleFS
  Serial.println("Mounting LittleFS...");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS. Formatting...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully.");
      if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS after formatting.");
        return;
      }
    } else {
      Serial.println("Failed to format LittleFS.");
      return;
    }
  } else {
    Serial.println("LittleFS mounted successfully");
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  camera_config_t config;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Use lower resolution and quality if PSRAM is not found
  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA; // Lower resolution
    config.jpeg_quality = 12;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_QVGA; // Further reduce resolution
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
  } else {
    Serial.println("Camera initialized successfully");
  }

  // Initialize the LED flash pin
  pinMode(LED_FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(LED_FLASH_GPIO_NUM, LOW); // Make sure the flash is off initially

  // Set pin modes for the ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Attach the servo motor to the specified pin
  myServo.attach(SERVO_PIN);
  // myServo.write(90); // Set initial position to 90 degrees (neutral)

}


void loop() {
  // Send a pulse to the TRIG pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance in centimeters
  distance = duration / 58.2;

  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print(" us, Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check for valid distance measurement
  if (distance < measurement && distance > 0) {
    delay(1000);
    capturePhotoSaveLittleFS();
    String prediction = sendPhoto();
    Serial.println("Prediction: " + prediction);

    // Debug statement to check if controlServo function is called with the right prediction
    Serial.println("Calling controlServo with prediction: " + prediction);
    controlServo(prediction);
  }

  delay(1000);
}


// Capture Photo and Save it to LittleFS
void capturePhotoSaveLittleFS(void) {
  // Turn on the flash
  digitalWrite(LED_FLASH_GPIO_NUM, HIGH);
  delay(100); // Wait for a brief moment to ensure the flash is on

  // Dispose first pictures because of bad quality
  camera_fb_t* fb = NULL;
  // Skip first 3 frames (increase/decrease number as needed).
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  // Take a new photo
  fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  // Turn off the flash
  digitalWrite(LED_FLASH_GPIO_NUM, LOW);

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}

String sendPhoto(void) {
  // Check WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl); // Specify destination URL

    File file = LittleFS.open(FILE_PHOTO_PATH, "r");
    if (!file) {
      Serial.println("Failed to open file for reading");
      return "";
    }

    size_t fileSize = file.size();
    uint8_t *buff = (uint8_t*) malloc(fileSize);
    file.read(buff, fileSize);
    file.close();

    http.setTimeout(20000); // Set timeout to 20 seconds
    http.addHeader("Content-Type", "image/jpeg"); // Specify content-type header
    http.addHeader("Content-Disposition", "form-data; name=\"file\"; filename=\"photo.jpg\"");

    int httpResponseCode = http.POST(buff, fileSize);
    free(buff);

    String prediction = "";

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Raw response: " + response);

      // Extract the prediction from the response
      response.replace("\"", "");
      response.trim(); // Trim any extraneous characters

      Serial.println("Trimmed response: " + response); // Debug statement
      if (response == "biodegradable" || response == "non-biodegradable") {
        prediction = response;
      }

      Serial.print("Prediction: ");
      Serial.println(prediction);

    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

    return prediction;
  } else {
    Serial.println("Error in WiFi connection");
    return "";
  }
}

void controlServo(String prediction) {
  // Debug statement to verify the function is entered
  Serial.println("Inside controlServo function with prediction: " + prediction);

  // Control the servo motor based on the prediction
  if (prediction == "biodegradable") {
    Serial.println("Moving servo to 180 degrees");
    myServo.write(140); // Rotate to 180 degrees (right)
    delay(1000); // Wait for the servo to move
    myServo.write(90); // Restore to initial position
    Serial.println("Servo restored to 90 degrees");
  } else if (prediction == "non-biodegradable") {
    Serial.println("Moving servo to 0 degrees");
    myServo.write(40); // Rotate to 0 degrees (left)
    delay(1000); // Wait for the servo to move
    myServo.write(90); // Restore to initial position
    Serial.println("Servo restored to 90 degrees");
  } else {
    myServo.write(90); // Default position
  }
}

