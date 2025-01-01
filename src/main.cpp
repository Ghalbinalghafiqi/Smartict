#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Fuzzy.h>

#define RELAY 4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RXD2 16
#define TXD2 17
PZEM004Tv30 pzem2(Serial2, RXD2, TXD2);

const char *ssid = "bin";
const char *password = "1234567890";

const size_t capacity = JSON_OBJECT_SIZE(50) + 1000;
char message[1024];

DynamicJsonDocument doc(capacity);
HTTPClient http;

float voltage1, current1, power1, energy1, frequency1, pf1, va1, VAR1;
float konsumsikwh_bulanlalu; // Variabel input
float konsumsikwh_bulanini; // Variabel input

float prediksipemakaian_listrik; // Variabel output

// Fuzzy *fuzzy;

// Instantiating a Fuzzy object
Fuzzy *fuzzy = new Fuzzy();

void setupDisplay();
void connectToNetwork();
void telePrintChatId();
void teleSendMessage(String payload);

void setup() {
  Serial.begin(115200);
  vTaskDelay(1000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 Gagal"));
    for (;;)
      ;
  }

  pinMode(RELAY, OUTPUT);

  display.display();
  delay(2000);

  connectToNetwork();
  delay(1000);

  telePrintChatId();
  delay(3000);
    // konsumsikwh_bulanlalu input
    FuzzyInput *konsumsikwh_bulanlalu = new FuzzyInput(1);
    FuzzySet *konsumsikwh_bulanlalurendah = new FuzzySet(0, 0, 400, 900);
    konsumsikwh_bulanlalu->addFuzzySet(konsumsikwh_bulanlalurendah);
    FuzzySet *konsumsikwh_bulanlalusedang = new FuzzySet(400, 900, 900, 1400);
    konsumsikwh_bulanlalu->addFuzzySet(konsumsikwh_bulanlalusedang);
    FuzzySet *konsumsikwh_bulanlalutinggi= new FuzzySet(900, 1400, 2200, 2200);
    konsumsikwh_bulanlalu->addFuzzySet(konsumsikwh_bulanlalutinggi);
    fuzzy->addFuzzyInput(konsumsikwh_bulanlalu);

    // konsumsikwh_bulanini input
    FuzzyInput *konsumsikwh_bulanini = new FuzzyInput(2);
    FuzzySet *konsumsikwh_bulaninirendah = new FuzzySet(0, 0, 16494, 40000);
    konsumsikwh_bulanini->addFuzzySet(konsumsikwh_bulaninirendah);
    FuzzySet *konsumsikwh_bulaninisedang = new FuzzySet(16494, 40000, 115325, 150000);
    konsumsikwh_bulanini->addFuzzySet(konsumsikwh_bulaninisedang);
    FuzzySet *konsumsikwh_bulaninitinggi = new FuzzySet(115325, 150000, 199284, 300000);
    konsumsikwh_bulanini->addFuzzySet(konsumsikwh_bulaninitinggi);
    fuzzy->addFuzzyInput(konsumsikwh_bulanini);

    ///////////////////////////////////////////////////

    // PrediksiPemakaian_listrik Output
    FuzzyOutput *PrediksiPemakaian_listrik = new FuzzyOutput(1);
    FuzzySet *PrediksiPemakaian_listrikhemat = new FuzzySet(0, 0, 30, 50);
    PrediksiPemakaian_listrik->addFuzzySet(PrediksiPemakaian_listrikhemat);
    FuzzySet *PrediksiPemakaian_listriknormal = new FuzzySet(40, 55, 55, 70);
    PrediksiPemakaian_listrik->addFuzzySet(PrediksiPemakaian_listriknormal);
    FuzzySet *PrediksiPemakaian_listrikboros = new FuzzySet(60, 80, 100, 100);
    PrediksiPemakaian_listrik->addFuzzySet(PrediksiPemakaian_listrikboros);
    fuzzy->addFuzzyOutput(PrediksiPemakaian_listrik);

    // 1. konsumsikwh_bulanlalurendahkonsumsikwh_bulaninirendah -> PrediksiPemakaian_listrikhemat
    FuzzyRuleAntecedent *konsumsikwh_bulanlalurendahkonsumsikwh_bulaninirendah1 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalurendahkonsumsikwh_bulaninirendah1->joinWithAND(konsumsikwh_bulanlalurendah, konsumsikwh_bulaninirendah);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikhemat1 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikhemat1->addOutput(PrediksiPemakaian_listrikhemat);

    FuzzyRule *fuzzyRule1 = new FuzzyRule(1, konsumsikwh_bulanlalurendahkonsumsikwh_bulaninirendah1, PrediksiPemakaian_listrikhemat1);
    fuzzy->addFuzzyRule(fuzzyRule1);

    // 2. konsumsikwh_bulanlalurendahkonsumsikwh_bulaninisedang -> PrediksiPemakaian_listrikhemat2
    FuzzyRuleAntecedent *konsumsikwh_bulanlalurendahkonsumsikwh_bulaninisedang2 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalurendahkonsumsikwh_bulaninisedang2->joinWithAND(konsumsikwh_bulanlalurendah, konsumsikwh_bulaninisedang);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikhemat2 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikhemat2->addOutput(PrediksiPemakaian_listrikhemat);

    FuzzyRule *fuzzyRule2 = new FuzzyRule(2, konsumsikwh_bulanlalurendahkonsumsikwh_bulaninisedang2, PrediksiPemakaian_listrikhemat2);
    fuzzy->addFuzzyRule(fuzzyRule2);

    // 3. konsumsikwh_bulanlalurendahkonsumsikwh_bulaninitinggi -> PrediksiPemakaian_listrikhemat3
    FuzzyRuleAntecedent *konsumsikwh_bulanlalurendahkonsumsikwh_bulaninitinggi3 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalurendahkonsumsikwh_bulaninitinggi3->joinWithAND(konsumsikwh_bulanlalurendah, konsumsikwh_bulaninitinggi);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikhemat3 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikhemat3->addOutput(PrediksiPemakaian_listrikhemat);

    FuzzyRule *fuzzyRule3 = new FuzzyRule(3, konsumsikwh_bulanlalurendahkonsumsikwh_bulaninitinggi3, PrediksiPemakaian_listrikhemat3);
    fuzzy->addFuzzyRule(fuzzyRule3);

    // 4. konsumsikwh_bulanlalusedangkonsumsikwh_bulaninirendah -> PrediksiPemakaian_listriknormal4
    FuzzyRuleAntecedent *konsumsikwh_bulanlalusedangkonsumsikwh_bulankinirendah4 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalusedangkonsumsikwh_bulankinirendah4->joinWithAND(konsumsikwh_bulanlalusedang, konsumsikwh_bulaninirendah);

    FuzzyRuleConsequent *PrediksiPemakaian_listriknormal4 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listriknormal4->addOutput(PrediksiPemakaian_listriknormal);

    FuzzyRule *fuzzyRule4 = new FuzzyRule(4, konsumsikwh_bulanlalusedangkonsumsikwh_bulankinirendah4, PrediksiPemakaian_listriknormal4);
    fuzzy->addFuzzyRule(fuzzyRule4);

    // 5. konsumsikwh_bulanlalusedangkonsumsikwh_bulaninisedang -> PrediksiPemakaian_listriknormal5
    FuzzyRuleAntecedent *dayasedangbiayasedang5 = new FuzzyRuleAntecedent();
    dayasedangbiayasedang5->joinWithAND(konsumsikwh_bulanlalusedang, konsumsikwh_bulaninisedang);

    FuzzyRuleConsequent *PrediksiPemakaian_listriknormal5 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listriknormal5->addOutput(PrediksiPemakaian_listriknormal);

    FuzzyRule *fuzzyRule5 = new FuzzyRule(5, dayasedangbiayasedang5, PrediksiPemakaian_listriknormal5);
    fuzzy->addFuzzyRule(fuzzyRule5);

    // 6. konsumsikwh_bulanlalusedangkonsumsikwh_bulaninitinggi -> PrediksiPemakaian_listrikboros6
    FuzzyRuleAntecedent *konsumsikwh_bulanlalusedangkonsumsikwh_bulaninitinggi6 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalusedangkonsumsikwh_bulaninitinggi6->joinWithAND(konsumsikwh_bulanlalusedang, konsumsikwh_bulaninitinggi);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikboros6 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikboros6->addOutput(PrediksiPemakaian_listrikboros);

    FuzzyRule *fuzzyRule6 = new FuzzyRule(6, konsumsikwh_bulanlalusedangkonsumsikwh_bulaninitinggi6, PrediksiPemakaian_listrikboros6);
    fuzzy->addFuzzyRule(fuzzyRule6);

    // 7. konsumsikwh_bulanlalutinggikonsumsikwh_bulaninirendah -> PrediksiPemakaian_listrikboros7
    FuzzyRuleAntecedent *konsumsikwh_bulanlalutinggikonsumsikwh_bulaninirendah7 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalutinggikonsumsikwh_bulaninirendah7->joinWithAND(konsumsikwh_bulanlalutinggi, konsumsikwh_bulaninirendah);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikboros7 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikboros7->addOutput(PrediksiPemakaian_listrikboros);

    FuzzyRule *fuzzyRule7 = new FuzzyRule(7, konsumsikwh_bulanlalutinggikonsumsikwh_bulaninirendah7, PrediksiPemakaian_listrikboros7);
    fuzzy->addFuzzyRule(fuzzyRule7);

    // 8. konsumsikwh_bulanlalutinggikonsumsikwh_bulaninisedang -> PrediksiPemakaian_listrikboros8
    FuzzyRuleAntecedent *konsumsikwh_bulanlalutinggikonsumsikwh_bulaninisedang8 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalutinggikonsumsikwh_bulaninisedang8->joinWithAND(konsumsikwh_bulanlalutinggi, konsumsikwh_bulaninisedang);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikboros8 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikboros8->addOutput(PrediksiPemakaian_listrikboros);

    FuzzyRule *fuzzyRule8 = new FuzzyRule(8, konsumsikwh_bulanlalutinggikonsumsikwh_bulaninisedang8, PrediksiPemakaian_listrikboros8);
    fuzzy->addFuzzyRule(fuzzyRule8);

    // 9. konsumsikwh_bulanlalutinggikonsumsikwh_bulaninitinggi -> PrediksiPemakaian_listrikboros9
    FuzzyRuleAntecedent *konsumsikwh_bulanlalutinggikonsumsikwh_bulaninitinggi9 = new FuzzyRuleAntecedent();
    konsumsikwh_bulanlalutinggikonsumsikwh_bulaninitinggi9->joinWithAND(konsumsikwh_bulanlalutinggi, konsumsikwh_bulaninitinggi);

    FuzzyRuleConsequent *PrediksiPemakaian_listrikboros9 = new FuzzyRuleConsequent();
    PrediksiPemakaian_listrikboros9->addOutput(PrediksiPemakaian_listrikboros);

    FuzzyRule *fuzzyRule9 = new FuzzyRule(9, konsumsikwh_bulanlalutinggikonsumsikwh_bulaninitinggi9, PrediksiPemakaian_listrikboros9);
    fuzzy->addFuzzyRule(fuzzyRule9);
}

float zeroIfNan(float v) {
  if (isnan(v)) {
    v = 0;
  }
  return v;
}

void loop() {
  voltage1 = pzem2.voltage();
  voltage1 = zeroIfNan(voltage1);
  current1 = pzem2.current();
  current1 = zeroIfNan(current1);
  power1 = pzem2.power(); // W
  power1 = zeroIfNan(power1);
  energy1 = pzem2.energy();  // kWh
  energy1 = zeroIfNan(energy1);

  String status = (power1 > 425.00) ? "Boros" : "Normal";
  String keterangan = (digitalRead(RELAY) == HIGH) ? "Power Off" : "Power On";

  if (voltage1 == 0) {
    status = "Tidak ada Tegangan yang masuk";
    keterangan = "Power OFF";
    digitalWrite(RELAY, LOW); // Kontrol relay
  } else if (power1 > 0 && power1 < 425.00) {
    status = "Normal";
    keterangan = "Power ON";
    digitalWrite(RELAY, HIGH); // Kontrol relay
  } else if (power1 > 425.00) {
    status = "Boros";
    keterangan = "Power OFF";
    digitalWrite(RELAY, LOW); // Kontrol relay
    delay(15000);
  } else if (voltage1 >= 1.00 && power1 == 0) {
    status = "Tidak ada Daya yang masuk";
    keterangan = "Power ON";
    digitalWrite(RELAY, HIGH); // Kontrol relay
  }

  // Fuzzy loop
 konsumsikwh_bulanlalu = power1; // or any other suitable value
  
 konsumsikwh_bulanini = energy1 * 1352; // or any other suitable value

  fuzzy->setInput(1, konsumsikwh_bulanlalu);
  fuzzy->setInput(2, konsumsikwh_bulanini);
  fuzzy->fuzzify();

  float PrediksiPemakaian_listrikboros = fuzzy->defuzzify(1);

  Serial.print("\n===============================\n");
  Serial.print("Daya: ");
  Serial.println(konsumsikwh_bulanlalu);
  Serial.print("Biaya: ");
  Serial.println(konsumsikwh_bulanini);
  Serial.print("Pemakaian Listrik: ");
  Serial.println(PrediksiPemakaian_listrikboros);
  Serial.print(" ");
  Serial.printf("\n");
  Serial.print("---------- Informasi ----------");
  delay(1000);

  Serial.println("");
  Serial.printf("Voltage        : %.2f V\n", voltage1);
  Serial.printf("Current        : %.2f A\n", current1);
  Serial.printf("Power Active   : %.2f W\n", power1);
  Serial.printf("Energy         : %.2f kWh\n", energy1);
  Serial.printf("Status         : %s\n", status.c_str());
  Serial.printf("Keterangan     : %s\n", keterangan.c_str());
  Serial.println("---------- END ----------");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.printf("Voltage   : %.2f V\n", voltage1);
  display.printf("Current   : %.2f A\n", current1);
  display.printf("Power RMS : %.2f W\n", power1);
  display.printf("Energy    : %.1f kWh\n", energy1);
  display.printf("Status    : %s\n", status.c_str());
  display.printf("Keterangan: %s\n", keterangan.c_str());
  display.display();
  delay(1000);

  // Kirim data ke Telegram
  doc.clear();
  doc["chat_id"] = 5260723107;
  doc["text"] = "Voltage      : " + String(voltage1) + "V\n"
                "Current      : " + String(current1) + "A\n"
                "Power Active : " + String(power1) + "W\n"
                "Energy       : " + String(energy1) + "kWh\n"
                "Status       : " + status + "\n"
                "Keterangan   : " + keterangan;

  serializeJson(doc, message);
  teleSendMessage(String(message));
}

void connectToNetwork() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }

  Serial.println("Connected to network");
}

void telePrintChatId() {
  http.begin(
      "https://api.telegram.org/bot6511538304:AAHxF-aqxsyKOPQWseUu5DttN6loA0hpADs/getUpdates");
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();

    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.printf("Response: %s\n", response.c_str());
  } else {
    Serial.printf("Error on sending POST: %d\n", httpResponseCode);
  }

  http.end();
}

void teleSendMessage(String payload) {
  Serial.printf("HTTP Payload: %s\n", payload.c_str());
  http.begin(
      "https://api.telegram.org/bot6511538304:AAHxF-aqxsyKOPQWseUu5DttN6loA0hpADs/sendMessage");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String response = http.getString();

    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.printf("Response: %s\n", response.c_str());
  } else {
    Serial.printf("Error on sending POST: %d\n", httpResponseCode);
  }

  http.end();
}