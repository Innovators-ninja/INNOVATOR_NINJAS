#include <Wire.h> // current
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Nexttech";
const char* password = "aditya2006";
const char* Gemini_Token = "AIzaSyD2jPHEZFwWWKNn5onhlaNJM4bKQIm_c7s";
const char* Gemini_Max_Tokens = "100";
String res = "";

// Timing variables for non-blocking blinking
unsigned long previousMillis = 0;
const long interval = 500; // Blink every 500ms
bool eyelidsClosed = false;
bool isHappy = true; // Tracks current facial expression, happy by default

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  display.clearDisplay();
  drawFace(true); // Start with a happy face
  display.display();
  delay(2000);
}

void loop() {
  // Blink the eyes non-blocking
  blinkEyes();

  // Handle Gemini bot question/answer functionality if data is available
  if (Serial.available()) {
    askGeminiQuestion();
  }
}

void blinkEyes() {
  unsigned long currentMillis = millis();
  
  // Check if it's time to toggle eyelid state
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Toggle eyelids open/close
    eyelidsClosed = !eyelidsClosed;
    drawEyelids(eyelidsClosed);
    display.display();
  }
}

void askGeminiQuestion() {
  // Read question from Serial input
  Serial.println("Ask your Question : ");
  while (Serial.available()) {
    char add = Serial.read();
    res += add;
    delay(1);
  }
  res.trim();
  
  // Determine the mood based on the input
  if (res.indexOf("sad") != -1) {
    isHappy = false; // Set to sad face
  } else if (res.indexOf("happy") != -1) {
    isHappy = true; // Set to happy face
  }
  
  // Draw the appropriate face
  display.clearDisplay();
  drawFace(isHappy);
  display.display();

  res = "\"" + res + "\"";
  Serial.print("Asking Your Question : ");
  Serial.println(res);

  HTTPClient https;
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(Gemini_Token))) {
    https.addHeader("Content-Type", "application/json");
    String payload = "{\"contents\": [{\"parts\":[{\"text\":" + res + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + String(Gemini_Max_Tokens) + "}}";
    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String response = https.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      String answer = doc["candidates"][0]["content"]["parts"][0]["text"];
      answer.trim();

      Serial.println("Here is your Answer: ");
      Serial.println(answer);
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.println("[HTTPS] Unable to connect");
  }
  res = "";
}

void drawFace(bool happy) {
  int radius = 10;
  int centerX = 64;
  int centerY = 40;

  // Draw mouth based on mood
  if (happy) {
    // Happy mouth (smiling arc)
    for (int angle = 20; angle <= 160; angle += 5) {
      int x = centerX + radius * cos(radians(angle));
      int y = centerY + radius * sin(radians(angle));
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  } else {
    // Sad mouth (frowning arc)
    for (int angle = 200; angle <= 340; angle += 5) {
      int x = centerX + radius * cos(radians(angle));
      int y = centerY + radius * sin(radians(angle));
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }

  // Draw eyes (the same for happy and sad faces)
  display.fillCircle(40, 20, 5, SSD1306_WHITE); // Left eye
  display.fillCircle(88, 20, 5, SSD1306_WHITE); // Right eye
}

void drawEyelids(bool closing) {
  int leftEyeX = 40;
  int leftEyeY = 20;
  int rightEyeX = 88;
  int rightEyeY = 20;

  if (closing) {
    // Draw eyelids covering the eyes (closing)
    display.fillRect(leftEyeX - 6, leftEyeY - 3, 12, 3, SSD1306_WHITE);
    display.fillRect(rightEyeX - 6, rightEyeY - 3, 12, 3, SSD1306_WHITE);
  } else {
    // Clear eyelids (opening)
    display.fillRect(leftEyeX - 6, leftEyeY - 3, 12, 3, SSD1306_BLACK);
    display.fillRect(rightEyeX - 6, rightEyeY - 3, 12, 3, SSD1306_BLACK);
  }
}