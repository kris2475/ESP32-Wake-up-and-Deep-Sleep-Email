#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <time.h> 

// *** ADD THIS LINE FOR COMPATIBILITY ***
extern "C" {
    uint8_t temprature_sens_read(); 
}
// ***************************************

// ===================================
// 1. CONFIGURATION: YOUR CUSTOM DETAILS
// ===================================

// Wi-Fi Credentials
#define WIFI_SSID       "" 
#define WIFI_PASSWORD   "" 

// Gmail Sender Configuration (Using App Password)
#define SMTP_HOST         "smtp.gmail.com"
#define SMTP_PORT         465                 // SSL (recommended)
#define SENDER_EMAIL      "data.monitor.bot@gmail.com"
#define SENDER_PASSWORD   "" // YOUR 16-CHAR APP PASSWORD
#define SENDER_NAME       "10-Min Temp Log" 

// Recipient Configuration
#define RECIPIENT_EMAIL   "data.monitor.bot@gmail.com"
#define RECIPIENT_NAME    "Data Receiver"

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;      
const int   daylightOffset_sec = 3600; 

// Scheduling Constants
#define LOG_INTERVAL_MINUTES 10 
#define LOG_INTERVAL_SECONDS (LOG_INTERVAL_MINUTES * 60) 

// ===================================
// 2. GLOBAL VARIABLES & FUNCTION PROTOTYPES
// ===================================

SMTPSession smtp;
float currentTempC = 0.0;

// Variables stored in RTC memory survive Deep Sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR time_t nextCaptureTime = 0; 
RTC_DATA_ATTR bool initialTimeSet = false; 

// Conversion factor for micro seconds to seconds
#define uS_TO_S_FACTOR 1000000 

// Function Prototypes
void smtpCallback(SMTP_Status status);
void sendDataEmail(const char* subject, const char* message);
float readInternalTemperature();
void initTime();
void goToDeepSleep();
void connectWiFi();
time_t calculateNextCaptureTime(time_t currentTime);

// ===================================
// 3. SETUP (The Deep Sleep Main Logic)
// ===================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Check if we woke up from Deep Sleep or a fresh boot
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("\n--- Woke up from Deep Sleep (Timer) ---");
    } else {
        ++bootCount;
        Serial.printf("\n--- System Initial Boot Up #%d ---\n", bootCount);
    }

    // 1. Connect and Sync Time
    connectWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
        initTime(); // Sync time and set initial schedule using the flag
    } else {
        Serial.println("\nFailed to connect to Wi-Fi. Cannot sync time or send email.");
        // Sleep for 5 minutes and try the whole process again
        esp_sleep_enable_timer_wakeup(300 * uS_TO_S_FACTOR); 
        esp_deep_sleep_start();
    }

    // 2. Check Schedule (with 60-second safety buffer)
    time_t currentTime;
    time(&currentTime); 
    
    // Calculate difference: positive means seconds remaining until nextCaptureTime.
    long timeDifference = nextCaptureTime - currentTime; 
    
    Serial.printf("Next Log Time: %s", ctime(&nextCaptureTime));
    Serial.printf("Time Difference (seconds remaining): %ld\n", timeDifference);

    // LOG CONDITION: If past the scheduled time OR if within 60 seconds of the scheduled time.
    if (currentTime >= nextCaptureTime || (timeDifference > 0 && timeDifference < 60)) {
        Serial.println("\n--- STARTING SCHEDULED LOG CYCLE (10 MIN) [FORCED SEND] ---");
        
        // 3. Read Sensor
        currentTempC = readInternalTemperature();
        Serial.printf("Captured Temperature: %.2f °C\n", currentTempC);

        // 4. Send Email
        String subject = "10-Minute Temp Log: ";
        subject += String(currentTempC, 2);
        subject += " C";

        char timeStr[64];
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        String message = "Log captured at ";
        message += String(timeStr);
        message += ".<br>Internal Chip Temperature: **";
        message += String(currentTempC, 2);
        message += " °C**.";
        
        sendDataEmail(subject.c_str(), message.c_str());
        
        // Give time for email transmission
        delay(8000); 

        // 5. Calculate Next Sleep Cycle
        time_t timeAfterSend;
        time(&timeAfterSend); 
        nextCaptureTime = calculateNextCaptureTime(timeAfterSend);
        
        Serial.printf("New next log scheduled for: %s", ctime(&nextCaptureTime));
        
    } else {
        Serial.printf("Waking up only to check time. Next log is still in the future.\n");
    }
    
    // 6. Go to Sleep
    goToDeepSleep();
}

// ===================================
// 4. LOOP (Empty for Deep Sleep)
// ===================================

void loop() {
    // Empty - the chip sleeps after setup()
}

// ===================================
// 5. FUNCTION IMPLEMENTATIONS
// ===================================

void connectWiFi() {
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
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

time_t calculateNextCaptureTime(time_t currentTime) {
    struct tm *lt = localtime(&currentTime);
    
    long currentTotalSeconds = lt->tm_min * 60 + lt->tm_sec; 

    // Calculate the next minute interval (now multiples of 10: 00, 10, 20, etc.)
    int nextMinuteInterval = ((lt->tm_min / LOG_INTERVAL_MINUTES) + 1) * LOG_INTERVAL_MINUTES;
    long nextIntervalTotalSeconds = nextMinuteInterval * 60;

    long secondsToSleep;
    if (nextMinuteInterval >= 60) {
        secondsToSleep = 3600 - currentTotalSeconds; 
    } else {
        secondsToSleep = nextIntervalTotalSeconds - currentTotalSeconds;
    }
    
    if (secondsToSleep <= 0) {
        secondsToSleep = LOG_INTERVAL_SECONDS; 
    }
    
    return currentTime + secondsToSleep;
}

void initTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time from NTP. Cannot schedule log.");
    } else {
        Serial.println("Time synchronized successfully.");
        
        // Use the dedicated RTC flag to ensure scheduling only happens once.
        if (initialTimeSet == false) {
            time_t now;
            time(&now);
            
            nextCaptureTime = calculateNextCaptureTime(now);
            initialTimeSet = true; // Set the flag so this block is skipped on wake-up
            Serial.printf("Initial next log scheduled for: %s", ctime(&nextCaptureTime));
        }
    }
}

void goToDeepSleep() {
    time_t currentTime;
    time(&currentTime);

    long sleepTimeSec = nextCaptureTime - currentTime;

    if (sleepTimeSec <= 0) { 
        Serial.println("WARNING: Missed log time! Sleeping 60s to retry.");
        sleepTimeSec = 60; 
    }
    
    Serial.printf("Going into deep sleep for %ld seconds...\n", sleepTimeSec);
    esp_sleep_enable_timer_wakeup(sleepTimeSec * uS_TO_S_FACTOR);
    
    // Clean up before sleep
    WiFi.disconnect(true);
    Serial.flush(); 
    esp_deep_sleep_start();
}

// --------------------------------------------------------------------
// FIXED readInternalTemperature() FUNCTION (Forces a fresh reading)
// --------------------------------------------------------------------
float readInternalTemperature() {
    // We use the low-level, hidden function to force a fresh reading from the sensor.
    // This is necessary because temperatureRead() often caches the value across deep sleeps.
    uint8_t tempF_raw = temprature_sens_read();
    float tempF = (float)tempF_raw; 
    
    // Convert from Fahrenheit to Celsius
    float tempC = (tempF - 32.0) / 1.8;
    return tempC;
}
// --------------------------------------------------------------------


void sendDataEmail(const char* subject, const char* message) {
    // --- Configure and Connect for Email ---
    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST; 
    session.server.port = SMTP_PORT;
    session.login.email = SENDER_EMAIL;
    session.login.password = SENDER_PASSWORD;

    smtp.callback(smtpCallback);

    if (!smtp.connect(&session)) {
        Serial.print("Error connecting to SMTP server before send: ");
        Serial.println(smtp.errorReason());
        return; // Exit if connection fails
    }

    // --- Prepare and Send Email ---
    SMTP_Message email;
    email.sender.name = SENDER_NAME;
    email.sender.email = SENDER_EMAIL;
    email.subject = subject;
    email.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
    email.text.content = message;
    email.text.charSet = "UTF-8";
    email.text.transfer_encoding = Content_Transfer_Encoding::enc_base64; 

    if (!MailClient.sendMail(&smtp, &email, true)) {
        Serial.print("Error sending Email: ");
        Serial.println(smtp.errorReason());
    }
}

void smtpCallback(SMTP_Status status) {
    // This callback function remains the same as your previous working version.
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


