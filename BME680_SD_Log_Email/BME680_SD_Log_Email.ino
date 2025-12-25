#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <time.h>

// ===================================
// 1. CONFIGURATION
// ===================================
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

#define SMTP_HOST       "smtp.gmail.com"
#define SMTP_PORT       465 
#define SENDER_EMAIL    ""
#define SENDER_PASSWORD ""
#define RECIPIENT_EMAIL ""

// AeroGuard Hardware Pins
#define SD_CS_PIN       5
#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22

// Timing
const unsigned long LOG_INTERVAL = 5000;  // 5 seconds
const unsigned long EMAIL_INTERVAL = 60000; // 60 seconds

// File naming
const char* logFileName = "/bme680_log.csv";

// ===================================
// 2. GLOBALS & OBJECTS
// ===================================
SMTPSession smtp;
Adafruit_BME680 bme;
unsigned long lastLogTime = 0;
unsigned long lastEmailTime = 0;

void smtpCallback(SMTP_Status status);

// ===================================
// 3. SETUP
// ===================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Initializing AeroGuard System ---");

  // A. Initialize I2C and BME680
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  // Set up oversampling and filter
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(0, 0); // Heater off for simple T/P/H logging

  // B. Initialize SD Card (CS Pin 5)
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed!");
    while (1); 
  }
  
  // Create header for CSV
  File file = SD.open(logFileName, FILE_WRITE);
  if (file) {
    file.println("Uptime_ms,Temp_C,Hum_%,Pres_hPa");
    file.close();
  }

  // C. Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // D. Sync System Time (Required for Gmail SSL)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
  Serial.println("\nSystem Ready.");
}

// ===================================
// 4. MAIN LOOP
// ===================================
void loop() {
  unsigned long currentTime = millis();

  // Log data every 5 seconds
  if (currentTime - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = currentTime;

    if (bme.performReading()) {
      File file = SD.open(logFileName, FILE_APPEND);
      if (file) {
        file.printf("%lu,%.2f,%.2f,%.2f\n", 
                    currentTime, 
                    bme.temperature, 
                    bme.humidity, 
                    bme.pressure / 100.0);
        file.close();
        Serial.printf("Logged: %.2f C, %.2f hPa\n", bme.temperature, bme.pressure / 100.0);
      }
    }
  }

  // Send email every 60 seconds
  if (currentTime - lastEmailTime >= EMAIL_INTERVAL) {
    lastEmailTime = currentTime;
    Serial.println("Reaching 60s mark. Sending CSV via Email...");
    sendEmailWithAttachment();
    
    // Clear and reset file for the next 60-second batch
    File f = SD.open(logFileName, FILE_WRITE);
    f.println("Uptime_ms,Temp_C,Hum_%,Pres_hPa");
    f.close();
  }
}

// ===================================
// 5. EMAIL LOGIC
// ===================================
void sendEmailWithAttachment() {
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_PASSWORD;

  SMTP_Message message;
  message.sender.name = "AeroGuard BME680 Bot";
  message.sender.email = SENDER_EMAIL;
  message.subject = "BME680 Sensor Log - Last 60 Seconds";
  message.addRecipient("Admin", RECIPIENT_EMAIL);
  message.text.content = "Attached is the latest environmental data log.";

  SMTP_Attachment att;
  att.descr.filename = "bme680_log.csv";
  att.descr.mime = "text/csv";
  att.file.path = logFileName;
  att.file.storage_type = esp_mail_file_storage_type_sd; 
  
  message.addAttachment(att);
  smtp.callback(smtpCallback);

  if (!smtp.connect(&session)) return;

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error: " + smtp.errorReason());
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}
