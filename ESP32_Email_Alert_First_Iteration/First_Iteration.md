# 📧 ESP32 Gmail Alert System: Iteration 1 (Email Test)

This project provides a simple, foundational demonstration of how to configure an ESP32 to connect to a Wi-Fi network and send an email using Google's SMTP server. This initial iteration verifies all required credentials and library functionality before moving on to sensor integration and power-saving modes such as deep sleep.

---

## 📌 Project Status

| Component              | Status   | Notes                                   |
|------------------------|----------|-----------------------------------------|
| Wi-Fi Connection       | Working  | Connects automatically on power-up      |
| Email Service (Gmail)  | Working  | Sends one test email per reset/boot     |
| Sensor / Deep Sleep    | Not Included | Logic intentionally disabled       |

---

## Prerequisites

### Hardware
- ESP32 Development Board (e.g., ESP32 DevKitC)
- Micro-USB cable (for power and programming)

### Software & Libraries
- Arduino IDE
- ESP32 Board Support Package installed in Arduino IDE
- **ESP_Mail_Client** library  
  - Author: Mobizt  
  - Version: 3.4.24 or later  
  - Installed via Arduino Library Manager

---

## 🔑 Setup Guide: Google App Password

To use Gmail’s SMTP server, you must authenticate using a **Google App Password** instead of your primary account password. This is required because the ESP32 cannot handle modern web-based authentication protocols.

---

### 1. Enable 2-Step Verification (2FA)

The App Password feature is only available when 2-Step Verification is enabled.

1. Go to your **Google Account → Security** page.
2. Under **How you sign in to Google**, enable **2-Step Verification**.
3. Complete the setup using your phone or another verification method.

---

### 2. Generate the App Password

1. Return to **Google Account → Security**.
2. Navigate to:  
   **2-Step Verification → App passwords**
3. On the App Passwords page:
   - **Select app**: Mail  
   - **Select device**: Other (Custom name…)  
   - **Name**: `ESP32 Email Alert`
4. Click **Generate**.
5. A **16-character password** (e.g., `abcd efgh ijkl mnop`) will be displayed.
6. Copy this code immediately and store it securely.

This value will be used as `SENDER_PASSWORD` in the Arduino sketch.

---

## ⚙️ Sketch Configuration

Before uploading, replace all placeholder values in the configuration section of the sketch with your actual credentials and App Password.

---

### 1. Update Credentials

Edit the following constants in your Arduino sketch:

```cpp
// Wi-Fi Credentials
#define WIFI_SSID       "YOUR_WIFI_NETWORK_NAME"      // <-- Replace with your SSID
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"          // <-- Replace with your password

// Gmail Sender Configuration
// CRITICAL: This Gmail account MUST be the one used to generate the App Password.
#define SENDER_EMAIL      "YOUR_SENDER_GMAIL@gmail.com"
#define SENDER_PASSWORD   "YOUR16CHARAPPPASSWORD"     // <-- 16 characters, NO SPACES

// Recipient Configuration
#define RECIPIENT_EMAIL   "YOUR_RECIPIENT_EMAIL@example.com" // <-- Alert destination
