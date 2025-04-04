// 📲 Includes for WiFi, Telegram, Time, OLED
#include <ESP8266WiFi.h>            // 🌐 Connect ESP8266 to WiFi
#include <WiFiClientSecure.h>       // 🔐 Secure client for HTTPS requests (Telegram)
#include <UniversalTelegramBot.h>   // 🤖 Telegram Bot library for alerts
#include <WiFiUdp.h>                // ⏳ UDP for NTP (Time sync)
#include <NTPClient.h>              // 📆 Get current time from internet
#include <ESP8266HTTPClient.h>      // 🌍 HTTP client to communicate with ESP32-CAM
#include <time.h>                   // ⏰ C-style time functions
#include <Wire.h>                   // 🔌 I2C communication for OLED
#include <Adafruit_SSD1306.h>       // 📟 OLED display library
#include <Adafruit_GFX.h>           // 🖼 Graphics support for OLED
#include "config.h"                 // 🔧 Custom configuration file (for SSID, Password, Bot Token)

// 📌 Pin Definitions
#define TRIG_PIN D5           // 📤 Ultrasonic Trigger pin
#define ECHO_PIN D6           // 📥 Ultrasonic Echo pin
#define BUZZER_PIN D7         // 🔊 Buzzer pin
#define LED_PIN D8            // 💡 LED pin
#define SDA_PIN D2            // 🟡 OLED SDA
#define SCL_PIN D1            // 🟢 OLED SCL

// 🎯 Distance threshold (in cm) to detect intruder
#define DETECTION_THRESHOLD 50

// 🖥 OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 🔒 Telegram Bot Client with Secure Connection
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// ⏱ Time Setup using NTP
WiFiUDP ntpUDP;
const long GMT_OFFSET = 5 * 3600 + 30 * 60; // 🕔 Offset for IST (UTC+5:30)
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// 🏠 ESP32-CAM IP address (you must change this to your ESP32-CAM IP)
const char* espCamIP = "http://192.168.1.100/capture";

// 🗓 Get current formatted date as string
String getFormattedDate() {
timeClient.update(); // 🔄 Sync NTP
time_t rawTime = timeClient.getEpochTime() + GMT_OFFSET; // 🔁 Adjust for IST
struct tm *timeInfo = gmtime(&rawTime);  // 🧠 Get time info
char buffer[20];
strftime(buffer, sizeof(buffer), "📅 %d/%m/%Y", timeInfo);  // 📅 Format date
return String(buffer);
}

// 🕒 Get current formatted time as string
String getFormattedTime() {
time_t rawTime = timeClient.getEpochTime() + GMT_OFFSET;
struct tm *timeInfo = gmtime(&rawTime);
char buffer[20];
strftime(buffer, sizeof(buffer), "⏳ %H:%M:%S", timeInfo);  // 🕒 Format time
return String(buffer);
}

// 📟 Display any two lines of text on OLED, centered horizontally
void updateDisplay(String line1, String line2) {
display.clearDisplay();               // 🧹 Clear screen
display.setTextSize(2);               // 🔠 Font size
display.setTextColor(WHITE);          // ⚪ Text color

// ➕ Center Line 1
int16_t x1, y1;
uint16_t w1, h1;
display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
int xPos1 = (SCREEN_WIDTH - w1) / 2;
display.setCursor(xPos1, 15);         // 🖥 Position line 1
display.print(line1);

// ➕ Center Line 2
uint16_t w2, h2;
display.getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
int xPos2 = (SCREEN_WIDTH - w2) / 2;
display.setCursor(xPos2, 40);         // 🖥 Position line 2
display.print(line2);

display.display();                    // 📟 Update OLED
}


// 📦 Send photo capture request to ESP32-CAM and forward to Telegram
void sendEspCamImage() {
WiFiClient camClient;
HTTPClient http;
  
Serial.println("📸 Capturing Image from ESP32-CAM...");
http.begin(camClient, espCamIP);     // 🌐 Begin HTTP GET request

int httpCode = http.GET();           // ⬇ Fetch image
if (httpCode == HTTP_CODE_OK) {
// ✅ Success: Send photo to Telegram bot
bot.sendPhoto(chatID, espCamIP, "📸 *Intruder Captured!*\n" + getFormattedDate() + "\n" + getFormattedTime());
} else {
Serial.println("❌ Failed to capture image! HTTP Code: " + String(httpCode));
}

http.end(); // 🧹 Cleanup
}

// 📏 Measure distance using ultrasonic sensor
long measureDistance() {
digitalWrite(TRIG_PIN, LOW);     // Set trigger low
delayMicroseconds(2);
digitalWrite(TRIG_PIN, HIGH);    // Send 10us pulse
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);

long duration = pulseIn(ECHO_PIN, HIGH, 20000);  // 🕰 Read echo duration
if (duration == 0) return -1;                    // ⚠ If timeout, return -1

long distance = duration * 0.034 / 2;            // 📏 Calculate distance in cm
return distance;
}

// 📩 Send a formatted message to Telegram bot
void sendTelegramMessage(String message) {
bot.sendMessage(chatID, message, "Markdown");
}

//---------------------------------- 🛠 Setup function -----------------------------------
void setup() {
Serial.begin(115200);  // 🖨 Begin serial monitor

// 🟢 Setup pins
pinMode(TRIG_PIN, OUTPUT);
pinMode(ECHO_PIN, INPUT);
pinMode(BUZZER_PIN, OUTPUT);
pinMode(LED_PIN, OUTPUT);

// 📡 Connect to WiFi
WiFi.begin(ssid, password);
Serial.print("Connecting to WiFi");
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("\n✅ WiFi Connected!");

client.setInsecure();         // ⚠ Allow insecure SSL for Telegram

timeClient.begin();           // 🕒 Start NTP
timeClient.forceUpdate();     // 🔄 Sync immediately

// 📟 Initialize OLED Display
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
Serial.println("❌ OLED init failed");
while (1);  // 🛑 Halt
}

updateDisplay("System Ready", "Monitoring..."); // 🖥 Welcome message

// 🔔 Telegram Boot Alert
  sendTelegramMessage("🛡️ *Security System Activated!*\n📡 WiFi: *Connected*\n" + getFormattedDate() + "\n" + getFormattedTime());
}

//------------------------------ 🔁 Loop function: runs continuously after setup-------------------------------------------------
void loop() {
long distance = measureDistance();  // 📏 Read sensor

if (distance > 0 && distance <= DETECTION_THRESHOLD) {
// 🚨 Intruder detected!
Serial.println("🚨 Security Breach!");

digitalWrite(BUZZER_PIN, HIGH);  // 🔊 Turn ON alarm
digitalWrite(LED_PIN, HIGH);     // 💡 Turn ON LED
delay(3000);                     // ⏳ Wait 3 seconds
digitalWrite(BUZZER_PIN, LOW);   // 🔇 Turn OFF
digitalWrite(LED_PIN, LOW);      
updateDisplay("SECURITY", "BREACH");  // 🖥 Update display

// 📩 Alert Message
String alertMessage = "🚨 *Intruder Alert!*\n";
alertMessage += "📍 Distance: " + String(distance) + " cm\n";
alertMessage += getFormattedDate() + "\n" + getFormattedTime();
sendTelegramMessage(alertMessage);  // 🔔 Send alert

sendEspCamImage();  // 📸 Capture and send image
} else {
// ✅ Safe zone
Serial.println("✅ Secured Area");
updateDisplay("SECURED", "");  // 🖥 Show secured status
}

delay(500); // 🕰 Wait half a second before next reading
}
