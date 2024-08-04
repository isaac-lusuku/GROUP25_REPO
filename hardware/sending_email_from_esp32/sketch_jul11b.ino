#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "LUSUKU_HOME_4G"
#define WIFI_PASSWORD "lusuku2024"

// SMTP server and port for Gmail
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_465

// Email login credentials
#define AUTHOR_EMAIL "lusuku2002@gmail.com"
#define AUTHOR_PASSWORD "xoyf mesf inwt size"

// Recipient email address
#define RECIPIENT_EMAIL "timothykalyango20@gmail.com"

// Declare the global SMTPSession object for SMTP transport
SMTPSession smtp;

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
  while (!Serial);
#endif

  Serial.println();

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  WiFiMulti multi;
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    // Print Wi-Fi status for debugging
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Set the network reconnection option
  MailClient.networkReconnect(true);

  // Enable the debug via Serial port
  smtp.debug(1);

  // Set the callback function to get the sending results
  smtp.callback(smtpCallback);

  // Declare the Session_Config for user-defined session credentials
  Session_Config config;

  // Set the session config
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = F("127.0.0.1");

  // Set the NTP config time
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  // Declare the message class
  SMTP_Message message;

  // Set the message headers
  message.sender.name = F("ESP32");
  message.sender.email = AUTHOR_EMAIL;
  message.addRecipient(F("Isaac"), RECIPIENT_EMAIL);

  // Set the email subject and message
  String subject = "This email is sent by an ESP32, it worked we shall use ESP32 so lets meet and you give me the equipments";
  message.subject = subject;

  String textMsg = "This email is sent by an ESP32, it worked we shall use ESP32 so lets meet and you give me the equipments";
  message.text.content = textMsg;

  // Set the content transfer encoding
  message.text.transfer_encoding = "base64"; // recommend for non-ASCII words in message.

  // Set the Plain text message character set
  message.text.charSet = F("utf-8"); // recommend for non-ASCII words in message.

  // Set the message priority
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_high;

  // Set the custom message header
  message.addHeader(F("Message-ID: <abcde.fghij@gmail.com>"));

  // Connect to the server
  if (!smtp.connect(&config)) {
    MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()) {
    Serial.println("Not yet logged in.");
  } else {
    if (smtp.isAuthenticated())
      Serial.println("Successfully logged in.");
    else
      Serial.println("Connected with no Auth.");
  }

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

  // Clear sending result log
  smtp.sendingResult.clear();
}

void loop() {
  // Do nothing
}

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status) {
  // Print the current status
  Serial.println(status.info());

  // Print the sending result
  if (status.success()) {
    Serial.println("----------------");
    MailClient.printf("Message sent success: %d\n", status.completedCount());
    MailClient.printf("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      // Get the result item
      SMTP_Result result = smtp.sendingResult.getItem(i);

      MailClient.printf("Message No: %d\n", i + 1);
      MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
      MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      MailClient.printf("Recipient: %s\n", result.recipients.c_str());
      MailClient.printf("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // Clear sending result as the memory usage will grow up
    smtp.sendingResult.clear();
  }
}
