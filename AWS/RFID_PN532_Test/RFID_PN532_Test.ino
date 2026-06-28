#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>

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
#ifndef D1
#define D1 5  // PN532 IRQ (GPIO5)
#endif
#ifndef D2
#define D2 4  // PN532 RESET (GPIO4)
#endif
#ifndef D7
#define D7 13 // Buzzer (GPIO13)
#endif

#define OLED_SDA D5
#define OLED_SCL D6
#define PN532_IRQ D1
#define PN532_RESET D2
#define BUZZER_PIN D7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

bool nfcDetected = false;

void showMessage(const char* title, const char* msg1, const char* msg2 = "") {
  display.clearDisplay();
  
  // Draw header line
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
  
  // Draw body message
  display.setCursor(0, 18);
  display.setTextSize(1);
  display.println(msg1);
  if (msg2 && msg2[0] != '\0') {
    display.setCursor(0, 32);
    display.println(msg2);
  }
  
  display.display();
}

void playBootSound() {
  tone(BUZZER_PIN, 523, 100); delay(100); // C5
  tone(BUZZER_PIN, 659, 100); delay(100); // E5
  tone(BUZZER_PIN, 784, 100); delay(100); // G5
  tone(BUZZER_PIN, 1047, 200); delay(200); // C6
  noTone(BUZZER_PIN);
}

void playScanSound() {
  tone(BUZZER_PIN, 2000, 80); delay(100);
  tone(BUZZER_PIN, 2000, 80); delay(80);
  noTone(BUZZER_PIN);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(BUZZER_PIN, OUTPUT);
  playBootSound();
  Serial.println(F("\n=== PN532 & OLED I2C Test Sketch ==="));

  // Initialize shared I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println(F("I2C bus initialized."));

  // 1. Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED SSD1306 initialization failed!"));
    for (;;); // Stop execution
  }
  display.clearDisplay();
  display.display();
  Serial.println(F("OLED display initialized."));
  showMessage("NFC RFID Test", "Booting...", "Initializing PN532...");

  // 2. Initialize PN532 NFC Module
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(F("Didn't find PN53x board! Check connection & DIP switches."));
    showMessage("ERROR", "PN532 NOT FOUND!", "Check DIP switches & pins");
    nfcDetected = false;
  } else {
    nfcDetected = true;
    Serial.print(F("Found chip PN5")); 
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print(F("Firmware ver. ")); 
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.'); 
    Serial.println((versiondata >> 8) & 0xFF, DEC);
    
    // Configure secure access module to read RFID tags
    nfc.SAMConfig();
    Serial.println(F("PN532 SAM configured. Waiting for RFID card..."));
    
    char firmwareStr[30];
    snprintf(firmwareStr, sizeof(firmwareStr), "FW Ver: %d.%d", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    showMessage("PN532 READY", "Waiting for card...", firmwareStr);
  }
}

void loop() {
  if (!nfcDetected) {
    // Retry initialization if not detected at boot
    delay(2000);
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (versiondata) {
      nfcDetected = true;
      nfc.SAMConfig();
      showMessage("PN532 READY", "Waiting for card...", "PN532 Detected!");
      Serial.println(F("PN532 detected dynamically!"));
    }
    return;
  }

  // Scan indicator (blinking text or dot on screen)
  static bool dotState = false;
  dotState = !dotState;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("RFID Scanner (I2C)"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
  
  display.setCursor(0, 20);
  display.println(F("Scan tag / card now"));
  
  display.setCursor(0, 40);
  display.print(F("Listening"));
  if (dotState) {
    display.print(F("..."));
  }
  display.display();

  // Variables to hold card data
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).
  // Non-blocking wait: 400 milliseconds timeout
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 400);
  
  if (success) {
    // Play card scan sound (double beep)
    playScanSound();
    
    // Card found!
    Serial.println(F("\n--- Card Found! ---"));
    Serial.print(F("UID Length: ")); 
    Serial.print(uidLength, DEC); 
    Serial.println(F(" bytes"));
    
    Serial.print(F("UID Value: "));
    nfc.PrintHex(uid, uidLength);
    
    // Display on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("CARD DETECTED!"));
    display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
    
    // Format UID as String
    char uidStr[30] = "";
    char temp[8];
    for (uint8_t i = 0; i < uidLength; i++) {
      snprintf(temp, sizeof(temp), "%02X ", uid[i]);
      strcat(uidStr, temp);
    }
    
    display.setCursor(0, 20);
    display.print(F("Len: "));
    display.print(uidLength);
    display.println(F(" bytes"));
    
    display.setCursor(0, 35);
    display.println(F("UID:"));
    display.setCursor(0, 48);
    display.println(uidStr);
    display.display();
    
    // Keep showing tag details for 2.5 seconds
    delay(2500);
  }
}
