#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ===================================
// 1. CONFIGURATION: YOUR CUSTOM DETAILS
// ===================================

// Wi-Fi Credentials
#define WIFI_SSID       "SKYYRMR7"              // Your Wi-Fi Network Name
#define WIFI_PASSWORD   "K2xWvDFZkuCh"          // Your Wi-Fi Password

// Gmail Sender Configuration (Using App Password)
#define SMTP_HOST         "smtp.gmail.com"
#define SMTP_PORT         465                   // SSL (recommended)
#define SENDER_EMAIL      "data.monitor.bot@gmail.com"
#define SENDER_PASSWORD   "qznrhhizewhfzrud"    // YOUR 16-CHAR APP PASSWORD
#define SENDER_NAME       "ESP32 Test Alert"

// Recipient Configuration
#define RECIPIENT_EMAIL   "data.monitor.bot@gmail.com" // Sending alert to the same account
#define RECIPIENT_NAME    "data.monitor.bot"

// ===================================
// 2. GLOBAL VARIABLES & FUNCTION PROTOTYPES
// ===================================

SMTPSession smtp;
bool emailSent = false;

// Function Prototypes
void smtpCallback(SMTP_Status status);
void sendAlertEmail(const char* subject, const char* message);

// ===================================
// 3. SETUP (Connect and Send Once)
// ===================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- Starting ESP32 Email Test ---");
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait up to 10 seconds for Wi-Fi connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // --- Configure and Send Email ---
    
    // 1. Setup Session Credentials
    ESP_Mail_Session session;
    
    // Fix 1: Use host_name instead of host
    session.server.host_name = SMTP_HOST; 
    session.server.port = SMTP_PORT;
    session.login.email = SENDER_EMAIL;
    session.login.password = SENDER_PASSWORD;

    smtp.callback(smtpCallback);

    // 2. Connect to SMTP Server
    // Fix 2: Must pass a pointer (&session) to the connect function
    if (smtp.connect(&session)) { 
        // 3. Send the Test Email
        sendAlertEmail(
            "SUCCESS: ESP32 Email Test (First Iteration)",
            "The ESP32 successfully connected to Wi-Fi and sent this test email using the Gmail App Password."
        );
        emailSent = true;
    } else {
        Serial.println("Error connecting to SMTP server.");
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
  delay(100);
}

// ===================================
// 5. HELPER FUNCTIONS
// ===================================

void sendAlertEmail(const char* subject, const char* message) {
  SMTP_Message email;

  email.sender.name = SENDER_NAME;
  email.sender.email = SENDER_EMAIL;
  email.subject = subject;
  email.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);

  email.text.content = message;
  email.text.charSet = "UTF-8";

  // Note: We use smtp.errorReason() here, which is often a separate function 
  // from the SMTP_Status object in the callback.
  if (!MailClient.sendMail(&smtp, &email, true)) {
    Serial.print("Error sending Email: ");
    Serial.println(smtp.errorReason());
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println("------------------------------------");
  Serial.println("SMTP Status Update:");
  
  // This line is already guaranteed to work as it was in the successful compile 
  // and simply prints status info.
  Serial.println(status.info()); 

  if (status.success()) {
    Serial.println("Email Sent Successfully!");
  } else {
    Serial.print("Error Details: ");
    // Final Fix: If status.info() failed to print the code, printing it a second 
    // time will still be safer than using a non-existent method.
    Serial.println(status.info()); 
  }
  Serial.println("------------------------------------");
}
