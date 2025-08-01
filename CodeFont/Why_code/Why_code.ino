#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Preferences.h>  // Persistent storage
#include <driver/ledc.h>
#include "Beon_Regular20pt7b.h"


#define TFT_DC 4
#define TFT_CS 15
#define TFT_RST 2
#define TFT_MISO 19         
#define TFT_MOSI 23           
#define TFT_CLK 18 

// Backlight control
#define BACKLIGHT_PIN 33
int backlightBrightness = 255;  // Default full brightness
bool backlightState = true;     // Default ON

// GPIO Pins for PWM control (removed pins 25, 33, 32)
const int pins[] = {14, 27, 26};
const int numPins = 3;
bool pinStates[numPins] = {false, false, false};
int pinBrightness[numPins] = {255, 255, 255};  // PWM brightness values (0-255)

// Initialize TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Generate unique SSID & Password
uint32_t chipID = (uint32_t)ESP.getEfuseMac();
String uniqueID = String(chipID & 0xFFFFFF, HEX);
String ssid = "ESP32_Display_" + uniqueID;
String password = "ESP_" + uniqueID;

// Persistent storage
Preferences preferences;

// Store text input
String line1;
String line2;

// Default text values
const String defaultLine1 = "WHY";
const String defaultLine2 = "2025";

// Start the web server
WiFiServer server(80);

// Function to save text to flash storage
void saveText(String text1, String text2) {
    preferences.begin("display", false);
    preferences.putString("line1", text1);
    preferences.putString("line2", text2);
    preferences.end();
}

// Function to save GPIO states and brightness
void savePinStates() {
    preferences.begin("display", false);
    for (int i = 0; i < numPins; i++) {
        preferences.putBool(("pin" + String(i)).c_str(), pinStates[i]);
        preferences.putInt(("brightness" + String(i)).c_str(), pinBrightness[i]);
    }
    preferences.end();
}

void handleTextUpdate(String request) {
    // Extract line1
    int line1Index = request.indexOf("line1=");
    int line2Index = request.indexOf("line2=");

    if (line1Index >= 0 && line2Index >= 0) {
        line1Index += 6; // Move past "line1="
        line2Index += 6; // Move past "line2="

        // Extract values from the request
        String newLine1 = request.substring(line1Index, request.indexOf('&', line1Index));
        String newLine2 = request.substring(line2Index, request.indexOf(' ', line2Index));

        // Decode URL encoding (replace %20 with space, etc.)
        newLine1.replace("%20", " ");
        newLine2.replace("%20", " ");

        // Save new text to persistent storage
        preferences.begin("display", false);
        preferences.putString("line1", newLine1);
        preferences.putString("line2", newLine2);
        preferences.end();

        // Update the display with new text
        line1 = newLine1;
        line2 = newLine2;
        updateDisplay();
    }
}

// Function to display Wi-Fi credentials
void showWiFiInfo() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    
    tft.setCursor(20, 80);
    tft.println("Connect to:");
    
    tft.setCursor(20, 110);
    tft.println(ssid);
    
    tft.setCursor(20, 150);
    tft.println("Password:");
    
    tft.setCursor(20, 180);
    tft.println(password);
}

void drawThickLine(int x1, int y, int x2, uint16_t color, int thickness) {
    for (int i = 0; i < thickness; i++) {
        tft.drawLine(x1, y + i, x2, y + i, color);
    }
}

void updateDisplay() {
    tft.setRotation(3);  // Landscape (320x240)
    tft.fillScreen(ILI9341_BLACK);
    tft.setFont(&Beon_Regular20pt7b);
    tft.setTextSize(0.5);

    // === LINE 1 (top) ===
    int16_t x1, y1;
    uint16_t w1, h1;
    tft.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
    int16_t textX1 = (320 - w1) / 2;
    int16_t textY1 = 110;
    tft.setTextColor(0xF25E95);
    tft.setCursor(textX1, textY1);
    tft.println(line1);

    // Main red line
    int16_t lineY1 = textY1 - h1 / 2 + 15;
    drawThickLine(0, lineY1, textX1 - 10, 0xF25E95, 5);       // Left part
    drawThickLine(textX1 + w1 + 10, lineY1, 320, 0xF25E95, 5); // Right part

    // Blue accents above the red line
    drawThickLine(0, lineY1 - 5 - 5, textX1 - 10, 0x61F2FF, 5);  // top left
    drawThickLine(textX1 + w1 + 10, lineY1 - 5 - 5, 320, 0x61F2FF, 5);  // top right


    // === LINE 2 (bottom, flipped) ===
    tft.setRotation((3 + 2) % 4);  // rotate 180
    tft.setFont(&Beon_Regular20pt7b);
    tft.setTextSize(0.5);

    tft.getTextBounds(line2, 0, 0, &x1, &y1, &w1, &h1);
    int16_t textX2 = (320 - w1) / 2;
    int16_t textY2 = 110;
    tft.setTextColor(0x61F2FF);
    tft.setCursor(textX2, textY2);
    tft.println(line2);

    int16_t lineY2 = textY2 - h1 / 2 + 15;

    // Main blue line
    drawThickLine(0, lineY2, textX2 - 10, 0x61F2FF, 5);
    drawThickLine(textX2 + w1 + 10, lineY2, 320, 0x61F2FF, 5);

    // Red accents above the blue line
    drawThickLine(0, lineY2 - 5 - 5, textX2 - 10, 0xF25E95, 5);  // top left
    drawThickLine(textX2 + w1 + 10, lineY2 - 5 - 5, 320, 0xF25E95, 5);  // top right

    // Reset
    tft.setRotation(3);
    tft.setFont(NULL);
}

void setup() {
    pinMode(BACKLIGHT_PIN, OUTPUT);
    Serial.begin(115200);

    SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
    // Initialize display
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);

    // Load saved text & backlight settings
    preferences.begin("display", false);
    line1 = preferences.getString("line1", defaultLine1);
    line2 = preferences.getString("line2", defaultLine2);
    backlightBrightness = preferences.getInt("brightness", 255);
    backlightState = preferences.getBool("backlight", true);
    preferences.end();

    // Load saved GPIO states and brightness
    preferences.begin("display", false);
    for (int i = 0; i < numPins; i++) {
        pinMode(pins[i], OUTPUT);
        pinStates[i] = preferences.getBool(("pin" + String(i)).c_str(), false);
        pinBrightness[i] = preferences.getInt(("brightness" + String(i)).c_str(), 255);
        setPinPWM(i, pinStates[i] ? pinBrightness[i] : 0);
    }
    preferences.end();

    setBacklight(255);

    // Show Wi-Fi credentials on screen
    showWiFiInfo();

    // Start Wi-Fi Access Point
    WiFi.softAP(ssid.c_str(), password.c_str());
    server.begin();

    // Wait 10 seconds before clearing screen
    //delay(10000);
    updateDisplay();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New Client Connected!");
        String request = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                request += c;

                if (c == '\n') {
                    // Handle Text Update
                    if (request.indexOf("GET /update") >= 0) {
                        handleTextUpdate(request);
                    }

                    // Handle GPIO Toggles
                    for (int i = 0; i < numPins; i++) {
                        if (request.indexOf("GET /toggle" + String(i)) >= 0) {
                            pinStates[i] = !pinStates[i];
                            setPinPWM(i, pinStates[i] ? pinBrightness[i] : 0);
                            savePinStates();
                        }
                    }

                    // Handle PWM Brightness Changes
                    for (int i = 0; i < numPins; i++) {
                        if (request.indexOf("GET /setBrightness" + String(i) + "?value=") >= 0) {
                            int brightnessValue = request.substring(request.indexOf("=") + 1).toInt();
                            if (brightnessValue >= 0 && brightnessValue <= 255) {
                                pinBrightness[i] = brightnessValue;
                                if (pinStates[i]) setPinPWM(i, pinBrightness[i]);
                                savePinStates();
                            }
                        }
                    }

                    // Handle Master PWM Brightness Change
                    if (request.indexOf("GET /setMasterBrightness?value=") >= 0) {
                        int brightnessValue = request.substring(request.indexOf("=") + 1).toInt();
                        if (brightnessValue >= 0 && brightnessValue <= 255) {
                            // Set all pins to the same brightness
                            for (int i = 0; i < numPins; i++) {
                                pinBrightness[i] = brightnessValue;
                                if (pinStates[i]) setPinPWM(i, pinBrightness[i]);
                            }
                            savePinStates();
                        }
                    }

                    // Handle "All On/Off" Button
                    if (request.indexOf("GET /toggleAll") >= 0) {
                        bool allOn = true;
                        for (int i = 0; i < numPins; i++) {
                            if (!pinStates[i]) allOn = false;
                        }
                        for (int i = 0; i < numPins; i++) {
                            pinStates[i] = !allOn;
                            setPinPWM(i, pinStates[i] ? pinBrightness[i] : 0);
                        }
                        savePinStates();
                    }

                    // Handle Backlight On/Off Toggle
                    if (request.indexOf("GET /toggleBacklight") >= 0) {
                        backlightState = !backlightState;
                        setBacklight(backlightState ? backlightBrightness : 0);
                        saveBacklightSettings();
                    }


                    // Handle Brightness Change
                    if (request.indexOf("GET /setBrightness?value=") >= 0) {
                        int brightnessValue = request.substring(request.indexOf("=") + 1).toInt();
                        if (brightnessValue >= 0 && brightnessValue <= 255) {
                            backlightBrightness = brightnessValue;
                            if (backlightState) setBacklight(backlightBrightness);
                            saveBacklightSettings();
                        }
                    }


                    // Send Web Page
                    sendWebPage(client);
                    break;
                }
            }
        }
        delay(10);
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

// Function to set PWM brightness for GPIO pins
void setPinPWM(int pinIndex, int brightness) {
    if (pinIndex >= 0 && pinIndex < numPins) {
        Serial.print("Setting GPIO ");
        Serial.print(pins[pinIndex]);
        Serial.print(" to brightness: ");
        Serial.println(brightness);
        analogWrite(pins[pinIndex], brightness);
    }
}

// Function to set the backlight brightness
void setBacklight(int brightness) {
    Serial.print("Setting Backlight to: ");
    Serial.println(brightness);
    analogWrite(BACKLIGHT_PIN, brightness);
}

void saveBacklightSettings() {
    preferences.begin("display", false);
    preferences.putInt("brightness", backlightBrightness);
    preferences.putBool("backlight", backlightState);
    preferences.end();
}

void sendWebPage(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><title>ESP32 TFT Control</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<style>body { font-family: Arial; text-align: center; } ");
    client.println("button { width: 80%; padding: 10px; margin: 10px; font-size: 20px; }");
    client.println("input { width: 80%; padding: 10px; margin: 10px; font-size: 20px; }</style></head><body>");

    client.println("<h2>ESP32 TFT Display</h2>");

    // Text Update Form
    client.println("<form action='/update' method='get'>");
    client.println("<input type='text' name='line1' placeholder='Line 1' value='" + line1 + "'><br>");
    client.println("<input type='text' name='line2' placeholder='Line 2' value='" + line2 + "'><br>");
    client.println("<button type='submit'>Update Display</button>");
    client.println("</form>");

    // Backlight Toggle
    client.println("<form action='/toggleBacklight' method='get'>");
    client.println("<button type='submit' style='background-color:" + String(backlightState ? "green" : "gray") + ";'>Toggle Backlight</button>");
    client.println("</form>");

    // Brightness Slider
    client.println("<label>Backlight Brightness: " + String(backlightBrightness) + "</label><br>");
    client.println("<input type='range' min='0' max='255' value='" + String(backlightBrightness) + "' onchange='updateBrightness(this.value)'><br>");

    // Master PWM Slider
    client.println("<h3>Master LED Brightness</h3>");
    client.println("<label>Set All LEDs to Same Brightness</label><br>");
    client.println("<input type='range' min='0' max='255' value='255' onchange='updateMasterBrightness(this.value)'><br>");
    client.println("<script>function updateBrightness(val) { var x = new XMLHttpRequest(); x.open('GET', '/setBrightness?value='+val, true); x.send(); }</script>");

    // GPIO Buttons with PWM sliders
    for (int i = 0; i < numPins; i++) {
        client.println("<h3>GPIO " + String(pins[i]) + "</h3>");
        
        // Toggle button
        client.println("<form action='/toggle" + String(i) + "' method='get'>");
        client.println("<button type='submit' style='background-color:" + String(pinStates[i] ? "green" : "gray") + ";'>GPIO " + String(pins[i]) + " (" + String(pinStates[i] ? "ON" : "OFF") + ")</button>");
        client.println("</form>");
        
        // Brightness slider
        client.println("<label>Brightness: " + String(pinBrightness[i]) + "</label><br>");
        client.println("<input type='range' min='0' max='255' value='" + String(pinBrightness[i]) + "' onchange='updatePinBrightness(" + String(i) + ", this.value)'><br>");
    }
    
    // JavaScript for PWM control
    client.println("<script>");
    client.println("function updateBrightness(val) { ");
    client.println("var x = new XMLHttpRequest(); ");
    client.println("x.open('GET', '/setBrightness?value='+val, true); ");
    client.println("x.send(); }");
    client.println("function updateMasterBrightness(val) { ");
    client.println("var x = new XMLHttpRequest(); ");
    client.println("x.open('GET', '/setMasterBrightness?value='+val, true); ");
    client.println("x.send(); ");
    client.println("setTimeout(function(){ location.reload(); }, 500); }"); // Refresh page to update individual sliders
    client.println("function updatePinBrightness(pin, val) { ");
    client.println("var x = new XMLHttpRequest(); ");
    client.println("x.open('GET', '/setBrightness'+pin+'?value='+val, true); ");
    client.println("x.send(); }");
    client.println("</script>");

    // Master Toggle
    client.println("<form action='/toggleAll' method='get'><button type='submit' style='background-color:blue;color:white;'>Toggle ALL GPIOs</button></form>");
    client.println("</body></html>");
}