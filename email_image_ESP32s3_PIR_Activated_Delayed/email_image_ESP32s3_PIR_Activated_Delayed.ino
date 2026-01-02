/* * ======================================================================================
 * PROJECT:      XIAO ESP32S3 Privacy-First Storeroom Monitor (v3.1)
 * HARDWARE:     Seeed Studio XIAO ESP32S3 + Camera/SD Sense Expansion Board
 * SENSOR:       PIR Motion Sensor (3.3V Logic)
 * * DESCRIPTION:
 * Privacy Mode: Once triggered, the camera stays "blind" for 5 minutes.
 * Anti-Flood: Sends only ONE email containing TWO timed captures (5m and 10m).
 * Stability:   Includes a "Wait-for-Quiet" loop to prevent instant re-triggering.
 * * --- LOGIC FLOW ---
 * 1. DEEP SLEEP: ESP32 waits for PIR signal on D1 (GPIO 2).
 * 2. TRIGGER: PIR goes HIGH -> ESP32 Wakes Up.
 * 3. DELAY 1: Waits 5 mins (Privacy window). CPU stays at 240MHz for USB stability.
 * 4. CAPTURE 1: Syncs NTP time, captures first photo, saves to SD.
 * 5. DELAY 2: Waits 5 more mins (Secondary verification).
 * 6. CAPTURE 2: Captures second photo, saves to SD.
 * 7. DELIVERY: Connects to WiFi, sends both photos in one email.
 * 8. WAIT FOR CLEAR: Stays awake until PIR pin goes LOW (person leaves room).
 * 9. RESET: Re-enters Deep Sleep.
 * * --- REQUIRED ARDUINO IDE "TOOLS" SETTINGS ---
 * Board:              "XIAO_ESP32S3"
 * PSRAM:              "OPI PSRAM"       <-- (CRITICAL: Camera will fail without this)
 * Flash Mode:         "QIO 80MHz"
 * Flash Size:         "8MB (64Mb)"
 * USB CDC On Boot:    "Enabled"         <-- (Allows Serial Monitor to work)
 * * --- WIRING ---
 * PIR VCC    -> XIAO 3V3
 * PIR GND    -> XIAO GND
 * PIR Signal -> XIAO D1 (GPIO 2)        
 * ======================================================================================
 */

#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "time.h"

// --- NETWORK & EMAIL CONFIG ---
#define WIFI_SSID       "SKYYRMR7"
#define WIFI_PASSWORD   "K2xWvDFZkuCh"
#define SENDER_EMAIL    "data.monitor.bot@gmail.com"
#define SENDER_PASSWORD "qznrhhizewhfzrud" 
#define RECIPIENT_EMAIL "data.monitor.bot@gmail.com"

// --- HARDWARE & TIMING ---
#define PIR_PIN          GPIO_NUM_2 
#define PRIVACY_DELAY    300000      // 5 Minutes (300,000ms)

SMTPSession smtp;
camera_fb_t *fb1 = NULL;
camera_fb_t *fb2 = NULL;
char ts1[25], ts2[25];

void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for Serial Monitor
  Serial.println("\n--- XIAO STORAGE MONITOR ACTIVATED ---");

  // 1. Camera Initialization
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
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed!");
    return;
  }

  // 2. Initial Delay (Privacy Window)
  Serial.println("Motion detected. Ignoring PIR for 5 mins...");
  delay(PRIVACY_DELAY);

  // 3. Sync Time & Capture Photo 1
  connectWiFi();
  fb1 = esp_camera_fb_get();
  getTimestamp(ts1);
  saveToSD(fb1, ts1, "1");
  Serial.print("Photo 1 Captured: "); Serial.println(ts1);

  // 4. Secondary Delay
  Serial.println("Waiting 5 more minutes...");
  delay(PRIVACY_DELAY);

  // 5. Capture Photo 2
  fb2 = esp_camera_fb_get();
  getTimestamp(ts2);
  saveToSD(fb2, ts2, "2");
  Serial.print("Photo 2 Captured: "); Serial.println(ts2);

  // 6. Deliver Data
  if(WiFi.status() == WL_CONNECTED) {
    sendDualPhotoEmail();
  }

  // 7. ANTI-LOOP: Wait for person to leave the room
  // This keeps the board from waking up the instant it goes to sleep
  Serial.println("Waiting for PIR to go LOW (Ready for next trigger)...");
  pinMode(PIR_PIN, INPUT);
  while(digitalRead(PIR_PIN) == HIGH) {
    delay(1000); 
  }
  Serial.println("Room clear.");

  // 8. Cleanup and Sleep
  if(fb1) esp_camera_fb_return(fb1);
  if(fb2) esp_camera_fb_return(fb2);
  
  WiFi.disconnect(true);
  esp_sleep_enable_ext0_wakeup(PIR_PIN, 1);
  Serial.println("Everything sent. Entering Deep Sleep.");
  esp_deep_sleep_start();
}

void connectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int r = 0;
  while (WiFi.status() != WL_CONNECTED && r < 20) {
    delay(500);
    Serial.print(".");
    r++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org");
    Serial.println(" Connected!");
  } else {
    Serial.println(" WiFi Timeout.");
  }
}

void getTimestamp(char* target) {
  time_t now; 
  time(&now);
  struct tm * ti = localtime(&now);
  strftime(target, 25, "%Y%m%d_%H%M%S", ti);
}

void saveToSD(camera_fb_t* fb, char* timestamp, const char* id) {
  if(!fb) return;
  if(SD_MMC.begin("/sdcard", true)) {
    String path = "/" + String(timestamp) + "_P" + id + ".jpg";
    File f = SD_MMC.open(path.c_str(), FILE_WRITE);
    if(f) { f.write(fb->buf, fb->len); f.close(); }
  }
}

void sendDualPhotoEmail() {
  Session_Config smtp_config;
  smtp_config.server.host_name = "smtp.gmail.com";
  smtp_config.server.port = 465;
  smtp_config.login.email = SENDER_EMAIL;
  smtp_config.login.password = SENDER_PASSWORD;

  SMTP_Message message;
  message.sender.name = "Storeroom Monitor";
  message.sender.email = SENDER_EMAIL;
  message.subject = "Storeroom Entry Alert";
  message.addRecipient("Admin", RECIPIENT_EMAIL);
  
  String body = "Trigger event detected.\n";
  body += "Capture 1 (5m Delay): " + String(ts1) + "\n";
  body += "Capture 2 (10m Delay): " + String(ts2);
  message.text.content = body.c_str();

  // Attach Pic 1
  SMTP_Attachment att1;
  att1.descr.filename = "pic1.jpg";
  att1.descr.mime = "image/jpeg";
  att1.blob.data = fb1->buf;
  att1.blob.size = fb1->len;
  att1.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att1);

  // Attach Pic 2
  SMTP_Attachment att2;
  att2.descr.filename = "pic2.jpg";
  att2.descr.mime = "image/jpeg";
  att2.blob.data = fb2->buf;
  att2.blob.size = fb2->len;
  att2.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att2);

  if (!smtp.connect(&smtp_config)) return;
  MailClient.sendMail(&smtp, &message);
}

void loop() {}
