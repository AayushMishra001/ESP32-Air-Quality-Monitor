#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// ==== CONFIG ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_BME280 bme; // I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
HardwareSerial pmSerial(2); // UART2 for PM sensor (GPIO16 RX, GPIO17 TX)

// WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// Global latest data
struct AQIData {
  String time;
  int pm1, pm25, pm10;
  float temperature, humidity, pressure;
  int aqi, adj_aqi;
} latest;

// AQI breakpoints (PM2.5 only)
struct Breakpoint {
  float Clow, Chigh;
  int Ilow, Ihigh;
};
Breakpoint aqiBreakpoints[] = {
  {0.0, 30.4, 0, 50},
  {30.5, 60.4, 51, 100},
  {60.5, 90.4, 101, 200},
  {90.5, 120.4, 201, 300},
  {120.5, 250.4, 301, 400},
  {250.5, 500.4, 401, 500}
};

// ====== AQI Calculation ======
int calculateAQI(float pm25) {
  for (auto bp : aqiBreakpoints) {
    if (pm25 >= bp.Clow && pm25 <= bp.Chigh) {
      float aqi = ((bp.Ihigh - bp.Ilow) / (bp.Chigh - bp.Clow)) * (pm25 - bp.Clow) + bp.Ilow;
      return round(aqi);
    }
  }
  if (pm25 > aqiBreakpoints[5].Chigh) return 500;
  return 0;
}

int adjustAQI(int aqi, float temperature, float humidity, float pressure) {
  float temp_dev = temperature - 25.0;
  float hum_dev = humidity - 50.0;
  float pres_dev = pressure - 1013.0;

  float correction = (-0.2 * temp_dev) + (0.1 * hum_dev) + (0.05 * pres_dev);
  int adjusted = round(aqi + correction);

  if (adjusted < 0) adjusted = 0;
  if (adjusted > 500) adjusted = 500;
  return adjusted;
}

// ====== PM Sensor Read ======
bool readPMFrame(int &pm1, int &pm25, int &pm10) {
  if (pmSerial.available() >= 32) {
    uint8_t buffer[32];
    pmSerial.readBytes(buffer, 32);

    if (buffer[0] == 0x42 && buffer[1] == 0x4D) {
      pm1  = (buffer[10] << 8) | buffer[11];
      pm25 = (buffer[12] << 8) | buffer[13];
      pm10 = (buffer[14] << 8) | buffer[15];
      return true;
    }
  }
  return false;
}

// ====== WebServer Handler with CORS ======
void handleData() {
  String json = "{";
  json += "\"time\":\"" + latest.time + "\",";
  json += "\"pm1\":" + String(latest.pm1) + ",";
  json += "\"pm25\":" + String(latest.pm25) + ",";
  json += "\"pm10\":" + String(latest.pm10) + ",";
  json += "\"temperature\":" + String(latest.temperature, 2) + ",";
  json += "\"humidity\":" + String(latest.humidity, 2) + ",";
  json += "\"pressure\":" + String(latest.pressure, 2) + ",";
  json += "\"aqi\":" + String(latest.aqi) + ",";
  json += "\"adj_aqi\":" + String(latest.adj_aqi);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", json);
}

// ====== OLED Update ======
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.printf("PM2.5: %d ug/m3", latest.pm25);

  display.setCursor(0, 10);
  display.printf("PM10 : %d ug/m3", latest.pm10);

  display.setCursor(0, 20);
  display.printf("Adj AQI: %d", latest.adj_aqi);

  display.setCursor(0, 30);
  display.printf("Temp: %.1f C", latest.temperature);

  display.setCursor(0, 40);
  display.printf("Hum : %.1f %%", latest.humidity);

  display.display();
}

void setup() {
  Serial.begin(115200);
  pmSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17

  // BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280!");
    while (1);
  }

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AQI Monitor Booting...");
  display.display();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  // Web server
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  int pm1, pm25, pm10;
  if (readPMFrame(pm1, pm25, pm10)) {
    float temp = bme.readTemperature();
    float hum  = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;

    int original_aqi = calculateAQI(pm25);
    int adjusted_aqi = adjustAQI(original_aqi, temp, hum, pres);

    latest.time = String(millis() / 1000) + "s";
    latest.pm1 = pm1;
    latest.pm25 = pm25;
    latest.pm10 = pm10;
    latest.temperature = temp;
    latest.humidity = hum;
    latest.pressure = pres;
    latest.aqi = original_aqi;
    latest.adj_aqi = adjusted_aqi;

    // Debug
    Serial.printf("[PM1=%d, PM2.5=%d, PM10=%d | T=%.2fC H=%.2f%% P=%.2fhPa | AQI=%d, AdjAQI=%d]\n",
                  pm1, pm25, pm10, temp, hum, pres, original_aqi, adjusted_aqi);

    // Update OLED
    updateDisplay();
  }

  server.handleClient();
}