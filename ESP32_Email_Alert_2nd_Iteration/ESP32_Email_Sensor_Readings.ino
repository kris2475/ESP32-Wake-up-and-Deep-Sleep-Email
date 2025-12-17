#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ===================================
// 1. CONFIGURATION: YOUR CUSTOM DETAILS
// ===================================

// Wi-Fi Credentials
#define WIFI_SSID       "" 
#define WIFI_PASSWORD   "" 

// Gmail Sender Configuration (Using App Password)
#define SMTP_HOST         "smtp.gmail.com"
#define SMTP_PORT         465                 // SSL (recommended)
#define SENDER_EMAIL      ""
#define SENDER_PASSWORD   "" // YOUR 16-CHAR APP PASSWORD
#define SENDER_NAME       "ESP32 Temp Monitor"

// Recipient Configuration
#define RECIPIENT_EMAIL   ""
#define RECIPIENT_NAME    "Data Receiver"

// ===================================
// 2. GLOBAL VARIABLES & FUNCTION PROTOTYPES
// ===================================

SMTPSession smtp;
float currentTempC = 0.0; // Global variable to hold temperature

// Function Prototypes
void smtpCallback(SMTP_Status status);
void sendAlertEmail(const char* subject, const char* message);
float readInternalTemperature(); // Function to read temperature

// ===================================
// 3. SETUP (Connect and Send Once)
// ===================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- Starting ESP32 Temperature Sensor Email Test ---");
  
  // Read the temperature 
  currentTempC = readInternalTemperature();
  Serial.printf("Internal Chip Temperature: %.2f °C\n", currentTempC);
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");

    // --- Configure and Send Email ---
    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST; 
    session.server.port = SMTP_PORT;
    session.login.email = SENDER_EMAIL;
    session.login.password = SENDER_PASSWORD;

    smtp.callback(smtpCallback);

    if (smtp.connect(&session)) { 
        
        // Prepare dynamic subject and message
        String subject = "Temperature Alert: ";
        subject += String(currentTempC, 2);
        subject += " C";

        String message = "The internal ESP32 chip temperature reading is **";
        message += String(currentTempC, 2);
        message += " °C**.";
        message += "<br>This email confirms data logging functionality.";
        
        // Send the Email
        sendAlertEmail(subject.c_str(), message.c_str());
        
    } else {
        Serial.print("Error connecting to SMTP server: ");
        Serial.println(smtp.errorReason());
    }
    
    // Give time for email transmission
    delay(3000);  

  } else {
    Serial.println("\nFailed to connect to Wi-Fi. Email not sent.");
  }
}

// ===================================
// 4. MAIN LOOP (Empty)
// ===================================

void loop() {
  // Empty for a simple one-time test
  delay(100);
}

// ===================================
// 5. HELPER FUNCTIONS
// ===================================

// Reads the internal temperature sensor of the ESP32 chip
float readInternalTemperature() {
    // This is the standard function call expected to work in your core version.
    float tempF = temperatureRead(); 

    // Convert Fahrenheit to Celsius
    float tempC = (tempF - 32.0) / 1.8;
    return tempC;
}

void sendAlertEmail(const char* subject, const char* message) {
  SMTP_Message email;

  email.sender.name = SENDER_NAME;
  email.sender.email = SENDER_EMAIL;
  email.subject = subject;
  email.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);

  email.text.content = message;
  email.text.charSet = "UTF-8";
  // Setting transfer encoding to HTML compatible base64
  email.text.transfer_encoding = Content_Transfer_Encoding::enc_base64; 

  if (!MailClient.sendMail(&smtp, &email, true)) {
    Serial.print("Error sending Email: ");
    Serial.println(smtp.errorReason());
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println("------------------------------------");
  Serial.println("SMTP Status Update:");
  Serial.println(status.info()); 

  if (status.success()) {
    Serial.println("Email Sent Successfully!");
  } else {
    Serial.print("Error Details: ");
    Serial.println(status.info()); 
  }
  Serial.println("------------------------------------");
}

