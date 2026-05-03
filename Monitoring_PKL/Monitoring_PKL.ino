
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6yF9AI0hY"
#define BLYNK_TEMPLATE_NAME "Monitoring Ruang Server"
#define BLYNK_AUTH_TOKEN "ubdHjeJHGm6fKZ3OQy4L16s44u1b_Fvl"

/* ========== LIBRARY ========== */
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <time.h>

/* ========== WIFI ========== */
char ssid[] = "ur moo";
char pass[] = "mimomimo";

/* ========== PIN ========== */
#define DHTPIN 16
#define DHTTYPE DHT11
#define MQ_PIN 34

/* ========== MQ CONFIG ========== */
#define RL_VALUE 10.0
#define RO_VALUE 10.0
#define ADC_MAX 4095.0
#define VCC 3.3

/* ========== OLED ========== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ========== OBJECT ========== */
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

/* ========== TIME (WIB) ========== */
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

/* ========== MQ → PPM ========== */
float calculatePPM(int adcValue) {
  if (adcValue < 1) adcValue = 1;

  float voltage = adcValue * (VCC / ADC_MAX);
  float rs = ((VCC - voltage) / voltage) * RL_VALUE;
  float ratio = rs / RO_VALUE;

  float ppm = pow(10, ((log10(ratio) - 1.027) / -0.662));

  if (ppm < 0) ppm = 0;
  if (ppm > 100000) ppm = 100000;

  return ppm;
}

/* ========== DATE ========== */
String getDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "--/--/----";

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y", &timeinfo);
  return String(buffer);
}

/* ========== SENSOR & DISPLAY ========== */
void sendSensor() {
  float suhu = dht.readTemperature();
  float hum  = dht.readHumidity();
  float ppm  = calculatePPM(analogRead(MQ_PIN));

  if (isnan(suhu) || isnan(hum)) {
    Serial.println("❌ DHT gagal dibaca");
    return;
  }

  /* ===== SERIAL MONITOR ===== */
  Serial.println("\n===== DATA SENSOR RUANG SERVER =====");
  Serial.print("Suhu       : "); Serial.print(suhu); Serial.println(" °C");
  Serial.print("Kelembaban : "); Serial.print(hum);  Serial.println(" %");
  Serial.print("Gas        : "); Serial.print(ppm);  Serial.println(" ppm");
  Serial.print("Tanggal    : "); Serial.println(getDate());
  Serial.println("===================================");

  /* ===== BLYNK ===== */
  Blynk.virtualWrite(V0, suhu);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, ppm);

  /* ===== NOTIFIKASI SUHU ===== */
  if (suhu > 27) {
    Blynk.logEvent("notifikasi_suhu",
      String("⚠️ Suhu Ruang Server Tinggi: ") + suhu + " °C");
  }

  /* ===== NOTIFIKASI GAS ===== */
  if (ppm >= 300 && ppm <= 700) {
    Blynk.logEvent("notifikasi_gas",
      String("⚠️ Terdeteksi ASAP: ") + ppm + " ppm");
  } else if (ppm > 700) {
    Blynk.logEvent("notifikasi_gas",
      String("🚨 ASAP/GAS TERDETEKSI: ") + ppm + " ppm");
  }

  /* ===== OLED ===== */
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(22, 0);
  display.println("MONITORING");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.print("Suhu : ");
  display.print(suhu, 1);
  display.println(" C");

  display.setCursor(0, 28);
  display.print("Hum  : ");
  display.print(hum, 0);
  display.println(" %");

  display.setCursor(0, 40);
  display.print("Gas  : ");
  display.print(ppm, 0);
  display.println(" ppm");

  display.drawLine(0, 52, 127, 52, SSD1306_WHITE);
  display.setCursor(30, 55);
  display.println(getDate());

  display.display();
}

/* ========== SETUP ========== */
void setup() {
  delay(1000);
  Serial.begin(115200);

  /* I2C */
  Wire.begin(21, 22);

  /* OLED INIT */
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED gagal");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 25);
  display.println("Monitoring");
  display.display();

  dht.begin();

  /* BLYNK */
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  /* TIME */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  timer.setInterval(2000L, sendSensor);

  sendSensor();   // 🔥 TAMBAHAN PENTING (OLED LANGSUNG UPDATE)

  Serial.println("✅ SISTEM SIAP");
}

/* ========== LOOP ========== */
void loop() {
  Blynk.run();
  timer.run();

  /* OLED KEEP ALIVE */
  static unsigned long oledRefresh = 0;
  if (millis() - oledRefresh > 3000) {
    oledRefresh = millis();
    display.display();
  }
}
