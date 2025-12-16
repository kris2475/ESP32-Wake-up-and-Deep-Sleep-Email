# ESP32 Email Alert System (Gmail SMTP)

This project demonstrates how to configure an ESP32 to send alert emails via Gmail using the **ESP_Mail_Client** library. It covers library installation, Gmail security configuration, a complete working Arduino sketch, and deployment steps.

---

## Prerequisites

- Arduino IDE (latest recommended)
- ESP32 board support installed in Arduino IDE
- A Gmail account with **2-Step Verification enabled**
- An active Wi-Fi network

---

## Step 1: Install the ESP_Mail_Client Library

### 1.1 Open Library Manager

In the Arduino IDE, navigate to:

```
Sketch > Include Library > Manage Libraries...
```

This opens the Library Manager window.

### 1.2 Search and Install

1. In the search bar, type **ESP_Mail_Client**.
2. Locate **ESP Mail Client** by **Mobizt**.
3. Click **Install**.

---

## Step 2: Configure Gmail for External Access (Critical)

Because the ESP32 is a non-Google application, you must use a **Gmail App Password**. Using your primary Gmail password is unsupported and insecure.

> **Note:** The interface may vary slightly, but the objective is to generate a **16-character App Password**.

### Steps

1. Go to your Google Account: https://myaccount.google.com
2. Open **Security**.
3. Under **Signing in to Google**, ensure **2-Step Verification** is **ON** (required).
4. Once enabled, select **App Passwords**.
5. Choose:
   - **App**: Mail
   - **Device**: Other / Custom (or default options)
6. Click **Generate**.
7. Copy the **16-character App Password** immediately.

This password will be used in your ESP32 sketch instead of your Gmail password.

---

## Step 3: Complete ESP32 Arduino Sketch

The sketch below includes Wi-Fi configuration, SMTP setup, and a basic alert trigger example.

```cpp
#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ------------------------------------
// 1. Wi-Fi Configuration
// ------------------------------------
#define WIFI_SSID       "YOUR_WIFI_NETWORK_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ------------------------------------
// 2. Sender Email Configuration
// ------------------------------------
#define SMTP_HOST         "smtp.gmail.com"
#define SMTP_PORT         465 // SSL (recommended)
#define SENDER_EMAIL      "data.monitor.bot@gmail.com"
#define SENDER_PASSWORD   "YOUR_16_CHAR_APP_PASSWORD"  // App Password
#define SENDER_NAME       "ESP32 Alert System"

// ------------------------------------
// 3. Recipient Email Configuration
// ------------------------------------
#define RECIPIENT_EMAIL   "kris.seunarine@example.com"
#define RECIPIENT_NAME    "Kris Seunarine"

SMTPSession smtp;
bool emailSent = false; // Prevent repeated emails

// Function prototypes
void smtpCallback(SMTP_Status status);
void sendAlertEmail(const char* subject, const char* message);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure SMTP session
  ESP_Mail_Session session;
  session.server.host = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_PASSWORD;
  session.login.user_domain = "";

  smtp.callback(smtpCallback);

  if (!smtp.connect(session)) {
    Serial.println("Error connecting to SMTP server.");
  }
}

void loop() {
  bool triggerConditionMet = false;

  // Example trigger: send alert after 10 seconds
  if (millis() > 10000 && !emailSent) {
    triggerConditionMet = true;
  }

  if (triggerConditionMet && !emailSent) {
    Serial.println("ALERT CONDITION MET! Sending email...");
    sendAlertEmail(
      "CRITICAL ALERT: System Threshold Exceeded",
      "The temperature sensor exceeded the safe limit. Immediate action required."
    );
    emailSent = true;
  }

  delay(1000);
}

void sendAlertEmail(const char* subject, const char* message) {
  SMTP_Message email;

  email.sender.name = SENDER_NAME;
  email.sender.email = SENDER_EMAIL;
  email.subject = subject;
  email.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);

  email.text.content = message;
  email.text.charSet = "UTF-8";
  email.text.transferEncoding = Content_Transfer_Encoding::enc_7bit;

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
    for (size_t i = 0; i < status.recipientCount(); i++) {
      Serial.printf("-> %s\n", status.recipient(i).c_str());
    }
  } else {
    Serial.printf("Error Code: %d | Reason: %s\n",
                  status.statusCode(), status.errorReason().c_str());
  }
  Serial.println("------------------------------------");
}
```

---

## Step 4: Customize and Upload

### Update Configuration

- Replace `YOUR_WIFI_NETWORK_NAME`
- Replace `YOUR_WIFI_PASSWORD`
- Replace `YOUR_16_CHAR_APP_PASSWORD` with your Gmail App Password
- Replace `kris.seunarine@example.com` with your recipient address

### Modify Alert Logic

Update the condition inside `loop()` to reflect your real sensor logic, for example:

```cpp
if (analogRead(A0) > 3000 && !emailSent)
```

### Select Board and Port

- **Tools > Board** → ESP32 Dev Module (or your specific ESP32 board)
- **Tools > Port** → Select the correct COM port

### Upload and Monitor

1. Click **Upload**.
2. Open **Serial Monitor**.
3. Set baud rate to **115200**.
4. Observe Wi-Fi connection status and email transmission logs.

---

## Notes and Best Practices

- Do not hard-code real credentials in public repositories.
- Implement rate limiting or cooldown timers to avoid email spam.
- Consider reconnect logic if SMTP or Wi-Fi drops.

---

## License

This project is provided as-is for educational and development purposes.
