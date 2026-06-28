#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Define custom pins with fallbacks for safety
#ifndef D5
#define D5 14 // OLED SDA (GPIO14)
#endif
#ifndef D6
#define D6 12 // OLED SCL (GPIO12)
#endif

#define OLED_SDA D5
#define OLED_SCL D6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi Credentials - User should replace these placeholders
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Day names
const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=== ESP8266 Wi-Fi NTP Clock ==="));

  // Initialize shared I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED SSD1306 initialization failed!"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to Wi-Fi (SSID: "));
  Serial.print(ssid);
  Serial.println(F(")..."));

  // Show connecting screen with animated dots
  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 5);
    display.println(F("Wi-Fi Connecting"));
    display.drawLine(0, 15, SCREEN_WIDTH - 1, 15, SSD1306_WHITE);

    display.setCursor(0, 25);
    display.print(F("SSID: "));
    display.println(ssid);

    display.setCursor(0, 45);
    display.print(F("Connecting"));
    for (int i = 0; i < dotCount; i++) {
      display.print(F("."));
    }
    
    display.display();
    dotCount = (dotCount + 1) % 5;
    delay(500);
    
    // Add serial indicator
    Serial.print(".");
  }

  Serial.println(F("\nWi-Fi Connected!"));
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("WiFi Connected!"));
  display.setCursor(0, 25);
  display.print(F("IP: "));
  display.println(WiFi.localIP().toString());
  display.setCursor(0, 45);
  display.println(F("Syncing NTP Time..."));
  display.display();
  delay(1500);

  // Configure SNTP Time (KST - Korea Standard Time: UTC+9, 9 * 3600 = 32400 seconds)
  // No daylight savings time (0 offset)
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println(F("NTP client configured."));
}

void loop() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  // Wait until time is synchronized (year is greater than 1970, meaning tm_year > 70)
  if (timeinfo->tm_year < 70) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 15);
    display.println(F("Synchronizing time"));
    display.setCursor(0, 30);
    display.println(F("with NTP server..."));
    
    // Draw animated loading lines/dots
    static int lineX = 0;
    display.drawLine(lineX, 50, lineX + 20, 50, SSD1306_WHITE);
    display.display();
    lineX = (lineX + 4) % (SCREEN_WIDTH - 20);
    
    delay(100);
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 1. Draw Header: WiFi Status & IP Address
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("WiFi: OK"));
  
  // Align IP address to the right
  String ipStr = WiFi.localIP().toString();
  int ipWidth = ipStr.length() * 6; // 6 pixels per character at size 1
  display.setCursor(SCREEN_WIDTH - ipWidth, 0);
  display.print(ipStr);
  
  // Draw header line
  display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);

  // 2. Draw Clock Time (Large, centered)
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  display.setTextSize(2);
  // Center alignment calculation: 
  // Each character at size 2 is 12 pixels wide (10 text + 2 padding), total width for 8 chars is 96 pixels.
  int clockX = (SCREEN_WIDTH - 96) / 2; 
  display.setCursor(clockX, 18);
  display.print(timeStr);

  // 3. Draw Date & Weekday (Small, centered)
  char dateStr[25];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d (%s)", 
           timeinfo->tm_year + 1900, 
           timeinfo->tm_mon + 1, 
           timeinfo->tm_mday, 
           dayNames[timeinfo->tm_wday]);
           
  display.setTextSize(1);
  int dateWidth = strlen(dateStr) * 6;
  int dateX = (SCREEN_WIDTH - dateWidth) / 2;
  display.setCursor(dateX, 42);
  display.print(dateStr);

  // 4. Draw Footer line & Seconds progress dot (Visual Animation)
  display.drawLine(0, 55, SCREEN_WIDTH - 1, 55, SSD1306_WHITE);
  
  // Traverse a dot along the footer line as seconds progress
  int dotX = (timeinfo->tm_sec * (SCREEN_WIDTH - 4)) / 59 + 2;
  display.fillCircle(dotX, 55, 2, SSD1306_WHITE);

  display.display();
  delay(200); // Check and refresh 5 times a second
}
