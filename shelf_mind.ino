#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include "HX711.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= DHT11 =================
#define DHTPIN  PA1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= RFID RC522 =================
#define SS_PIN  PA4
#define RST_PIN PA3
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ================= Sensors =================
#define LDR_PIN PA2
#define VIB_PIN PA8
#define BUZZER  PA0

// ================= LEDs =================
#define LED_GREEN  PB0    // Normal
#define LED_YELLOW PB11   // Warning
#define LED_RED    PB1    // Alert

// ================= Load Cells (HX711) =================
HX711 scale1, scale2, scale3;

#define DT1  PA11
#define SCK1 PA12

#define DT2  PB12
#define SCK2 PB13

#define DT3  PA15   // حسب السلايد
#define SCK3 PB3    // حسب السلايد

float cal = 1700;

// ================= Products =================
String products[3] = {"Vitamin C", "Antibiotics", "Skin Care"};

// ================= Loop control =================
int shelfIndex = 0;
unsigned long lastShelfTime = 0;
const unsigned long shelfDisplayDuration = 3000;  // 3 ثواني لكل رف

// ================= Functions =================
void beep(int t){
  digitalWrite(BUZZER, LOW);
  delay(t);
  digitalWrite(BUZZER, HIGH);
}

float readWeight(HX711 &scale){
  float w = scale.get_units(10);
  if (w < 5) w = 0;
  return w;
}

void controlLED(float w){
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);

  if (w > 200)      digitalWrite(LED_GREEN,  HIGH);
  else if (w > 20)  digitalWrite(LED_YELLOW, HIGH);
  else              digitalWrite(LED_RED,    HIGH);
}

String getUID(byte *uid, byte size) {
  String s = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// ================= Setup =================
void setup() {
  Wire.begin(PB7, PB6);

  lcd.init();
  delay(100);
  lcd.backlight();
  lcd.clear();

  dht.begin();

  pinMode(LDR_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);
  pinMode(BUZZER,  OUTPUT);
  digitalWrite(BUZZER, HIGH);

  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);

  SPI.begin();
  mfrc522.PCD_Init();

  scale1.begin(DT1, SCK1);
  scale2.begin(DT2, SCK2);
  scale3.begin(DT3, SCK3);

  scale1.set_scale(cal); scale1.tare();
  scale2.set_scale(cal); scale2.tare();
  scale3.set_scale(cal); scale3.tare();

  lcd.setCursor(0, 0);
  lcd.print("   Welcome In   ");
  lcd.setCursor(0, 1);
  lcd.print("   Shelf Mind   ");
  delay(2000);
  lcd.clear();

  lcd.print("  System Ready  ");
  delay(1500);
  lcd.clear();
}

// ================= LOOP =================
void loop() {

  // ================= RFID =================
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    beep(60);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CARD SCANNED");
    delay(800);

    String uid = getUID(mfrc522.uid.uidByte, mfrc522.uid.size);

    int shelf = -1;

    if (uid == "D4555A06") shelf = 0;
    if (uid == "9354AAD9") shelf = 1;
    if (uid == "1A216F06") shelf = 2;

    if (shelf != -1) {

      float weight = 0;

      if (shelf == 0) weight = readWeight(scale1);
      if (shelf == 1) weight = readWeight(scale2);
      if (shelf == 2) weight = readWeight(scale3);

      float t2 = dht.readTemperature();
      float h2 = dht.readHumidity();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SHELF ");
      lcd.print(shelf + 1);
      delay(1200);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(products[shelf]);
      delay(1200);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("W:");
      lcd.print(weight);
      delay(1200);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(t2);
      lcd.setCursor(0, 1);
      lcd.print("H:");
      lcd.print(h2);
      delay(1500);

      bool ok = (weight >= 20 &&
                 t2 <= 40.0 &&
                 h2 <= 70.0 &&
                 digitalRead(LDR_PIN) != LOW);

      lcd.clear();

      if (ok) {
        lcd.print("PRODUCT OK");
      } else {
        lcd.print("PRODUCT ERROR");
        beep(150);
      }

      delay(2000);
      lcd.clear();
    }

    mfrc522.PICC_HaltA();
  }

  // ================= Shelf Loop =================
  if (millis() - lastShelfTime > shelfDisplayDuration) {

    lastShelfTime = millis();

    int currentShelf = shelfIndex;

    float weight = 0;

    if (currentShelf == 0) weight = readWeight(scale1);
    if (currentShelf == 1) weight = readWeight(scale2);
    if (currentShelf == 2) weight = readWeight(scale3);

    controlLED(weight);

    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    lcd.setCursor(0, 0);
    lcd.print("Shelf ");
    lcd.print(currentShelf + 1);

    lcd.setCursor(0, 1);
    lcd.print(products[currentShelf]);
    lcd.print(" ");
    lcd.print(weight);
    lcd.print("g");

    shelfIndex++;
    if (shelfIndex > 2) shelfIndex = 0;
  }

  // ================= Monitoring =================
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  bool light = digitalRead(LDR_PIN);
  bool vib   = digitalRead(VIB_PIN);

  if (vib == LOW || t > 40.0 || h > 70.0 || light == LOW) {

    lcd.clear();

    if (vib == LOW) {
      lcd.setCursor(0, 0);
      lcd.print("VIBRATION!");
      beep(100);
    }

    if (t > 40.0) {
      lcd.setCursor(0, 0);
      lcd.print("HIGH TEMP!");
      beep(100);
    }

    if (h > 70.0) {
      lcd.setCursor(0, 0);
      lcd.print("HIGH HUM!");
      beep(100);
    }

    if (light == LOW) {
      lcd.setCursor(0, 0);
      lcd.print("SUNLIGHT!");
      beep(100);
    }

    delay(800);
  }
}
