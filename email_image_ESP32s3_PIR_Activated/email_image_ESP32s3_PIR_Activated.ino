/*
 * ======================================================================================
 * PROJECT: XIAO ESP32S3 Sense - PIR Triggered Security Camera (v2.0)
 * HARDWARE: Seeed Studio XIAO ESP32-S3 + Camera/SD Expansion Sense Board
 * FUNCTION: Captures a low-res image when PIR is triggered, saves to SD with 
 * date-time stamp, and emails via Gmail SMTP.
 * ======================================================================================
 * * --- REQUIRED ARDUINO IDE "TOOLS" SETTINGS ---
 * Board:              "XIAO_ESP32S3"
 * PSRAM:              "OPI PSRAM"       <-- (Critical for Camera Buffer)
 * Flash Mode:         "QIO 80MHz"       <-- (High-speed access)
 * Flash Size:         "8MB (64Mb)"
 * USB CDC On Boot:    "Enabled"         <-- (To see Serial Monitor output)
 * * --- LIBRARIES REQUIRED ---
 * 1. ESP Mail Client (by Mobizt) - Install via Library Manager
 * 2. ESP32 Camera (by Espressif) - Included with ESP32 Board Package
 * * --- WIRING ---
 * PIR VCC    -> XIAO 3V3
 * PIR GND    -> XIAO GND
 * PIR Signal -> XIAO D1 (GPIO 2)        <-- (Supports RTC Wakeup from Deep Sleep)
 * * --- GMAIL SETUP ---
 * 1. Enable 2-Factor Authentication (2FA) on your Google Account.
 * 2. Generate an "App Password" (Settings > Security > App Passwords).
 * 3. Use that 16-character code as your SENDER_PASSWORD below.
 * ======================================================================================
 */

#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "time.h"

// --- CONFIGURATION ---
#define WIFI_SSID       "SKYYRMR7"
#define WIFI_PASSWORD   "K2xWvDFZkuCh"
#define SENDER_EMAIL    "data.monitor.bot@gmail.com"
#define SENDER_PASSWORD "qznrhhizewhfzrud"
#define RECIPIENT_EMAIL "data.monitor.bot@gmail.com"

#define PIR_PIN GPIO_NUM_2  // D1 on XIAO ESP32S3

// NTP Settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // Adjust for your timezone (e.g., -18000 for EST)
const int   daylightOffset_sec = 3600;

SMTPSession smtp;

void setup() {
  Serial.begin(115200);

  // 1. Initialize Camera (XIAO ESP32S3 Sense specific pins)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 15; config.pin_d1 = 17; config.pin_d2 = 18; config.pin_d3 = 16;
  config.pin_d4 = 14; config.pin_d5 = 12; config.pin_d6 = 11; config.pin_d7 = 48;
  config.pin_xclk = 10; config.pin_pclk = 13; config.pin_vsync = 38;
  config.pin_href = 47; config.pin_sscb_sda = 40; config.pin_sscb_scl = 39;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // 2. Connect WiFi & Sync Time
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    retry++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) Serial.println("Time sync failed");
  }

  // 3. Capture Photo
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) return;

  // 4. Create Timestamp Filename
  time_t now;
  time(&now);
  struct tm * timeinfo = localtime(&now);
  char ts[25];
  // Format: YYYYMMDD_HHMMSS
  strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", timeinfo);
  String fileName = "/" + String(ts) + ".jpg";

  // 5. Save to SD Card
  if(SD_MMC.begin("/sdcard", true)) {
    File file = SD_MMC.open(fileName.c_str(), FILE_WRITE);
    if(file) {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.println("Saved: " + fileName);
    }
  }

  // 6. Send Email
  if(WiFi.status() == WL_CONNECTED) {
    sendPhotoEmail(fb, ts);
  }

  // 7. Cleanup and Deep Sleep
  esp_camera_fb_return(fb);
  
  // Set PIR (GPIO 2) as wakeup source
  esp_sleep_enable_ext0_wakeup(PIR_PIN, 1); // 1 = High/Triggered
  Serial.println("Entering Deep Sleep... Waiting for PIR");
  esp_deep_sleep_start();
}

void sendPhotoEmail(camera_fb_t * fb, char* ts) {
  Session_Config config;
  config.server.host_name = "smtp.gmail.com";
  config.server.port = 465;
  config.login.email = SENDER_EMAIL;
  config.login.password = SENDER_PASSWORD;

  SMTP_Message message;
  message.sender.name = "XIAO S3 Security";
  message.sender.email = SENDER_EMAIL;
  message.subject = "Motion Alert: " + String(ts);
  message.addRecipient("Admin", RECIPIENT_EMAIL);
  
  String msg = "Motion detected! Image captured at: " + String(ts);
  message.text.content = msg.c_str();

  SMTP_Attachment att;
  att.descr.filename = String(ts) + ".jpg";
  att.descr.mime = "image/jpeg";
  att.blob.data = fb->buf;
  att.blob.size = fb->len;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att);

  if (!smtp.connect(&config)) return;
  MailClient.sendMail(&smtp, &message);
}

void loop() {}
