/*
 * ======================================================================================
 * PROJECT: XIAO ESP32S3 Sense - Timed Security Camera (v1.0)
 * HARDWARE: Seeed Studio XIAO ESP32-S3 + Camera/SD Expansion Sense Board
 * FUNCTION: Captures a low-res image every 60s, saves to SD, and emails via Gmail SMTP.
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
 * * --- WIRING (For Future PIR Implementation) ---
 * PIR VCC    -> XIAO 3V3
 * PIR GND    -> XIAO GND
 * PIR Signal -> XIAO D1 (GPIO 2)        <-- (Supports RTC Wakeup from Deep Sleep)
 * * --- GMAIL SETUP ---
 * 1. Enable 2-Factor Authentication (2FA) on your Google Account.
 * 2. Generate an "App Password" (Settings > Security > App Passwords).
 * 3. Use that 16-character code as your SENDER_PASSWORD below.
 * * --- SD CARD ---
 * - Format: FAT32
 * - Interface: SD_MMC (Automatic on Sense board)
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

#define TIME_TO_SLEEP  60          // Time ESP32 will at rest (in seconds)
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds

SMTPSession smtp;

void setup() {
  Serial.begin(115200);

  // 1. Camera Initialization (XIAO ESP32S3 Pins)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 15; config.pin_d1 = 17; config.pin_d2 = 18; config.pin_d3 = 16;
  config.pin_d4 = 14; config.pin_d5 = 12; config.pin_d6 = 11; config.pin_d7 = 48;
  config.pin_xclk = 10; config.pin_pclk = 13; config.pin_vsync = 38;
  config.pin_href = 47; config.pin_sscb_sda = 40; config.pin_sscb_scl = 39;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA; // Low resolution
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera failed!");
    return;
  }

  // 2. Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // 3. Sync Time (NTP)
  configTime(0, 0, "pool.ntp.org");
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // 4. Capture Photo
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Capture failed");
    return;
  }

  // 5. Save to SD Card
  if(SD_MMC.begin("/sdcard", true)) {
    String path = "/" + String(millis()) + ".jpg"; // Simple filename for now
    File file = SD_MMC.open(path.c_str(), FILE_WRITE);
    if(file) {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.println("Saved to SD");
    }
  }

  // 6. Send Email
  sendPhotoEmail(fb);

  // 7. Cleanup and Sleep
  esp_camera_fb_return(fb);
  
  Serial.println("Sleeping for 60 seconds...");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void sendPhotoEmail(camera_fb_t * fb) {
  Session_Config config;
  config.server.host_name = "smtp.gmail.com";
  config.server.port = 465;
  config.login.email = SENDER_EMAIL;
  config.login.password = SENDER_PASSWORD;

  SMTP_Message message;
  message.sender.name = "XIAO S3 Cam";
  message.sender.email = SENDER_EMAIL;
  message.subject = "Timed Photo Update";
  message.addRecipient("User", RECIPIENT_EMAIL);
  message.text.content = "Attached is your 60-second interval photo.";

  SMTP_Attachment att;
  att.descr.filename = "photo.jpg";
  att.descr.mime = "image/jpeg";
  att.blob.data = fb->buf;
  att.blob.size = fb->len;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att);

  if (!smtp.connect(&config)) return;
  MailClient.sendMail(&smtp, &message);
}

void loop() {
  // Loop stays empty because Deep Sleep restarts the board.
}
