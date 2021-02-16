// Coding By IOXhop : http://www.ioxhop.com/

#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <EEPROM.h>
#include <Wire.h>
#include "DHT.h"
#include "dw_font.h"
#include "SSD1306.h"
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET  4

#define EEPROM_SIZE 8

//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BME280 bme;

SSD1306   display(0x3c, 21, 22);

extern dw_font_info_t font_th_sarabunpsk_regular40;
dw_font_t myfont;

void draw_pixel(int16_t x, int16_t y)
{
  display.setColor(WHITE);
  display.setPixel(x, y);
}

void clear_pixel(int16_t x, int16_t y)
{
  display.setColor(BLACK);
  display.setPixel(x, y);
}

// Update these with values suitable for your network.
const char* ssid = "cckarn._";
const char* password = "1909802728861";

// Config MQTT Server
#define mqtt_server "192.168.43.5"
#define mqtt_port 1883
#define mqtt_user "TEST"
#define mqtt_password "12345"

int LED_PIN = 2;
int LED_PIN2 = 27;
int pump = 4;
int ldr = 34;
int button = 32;
int startstate = 0;

const int Res = 8;
const int freq = 5000;
const int ledcchannel = 0;

int timezone = 7 * 3600;
int dst = 0;

#define DHTTYPE DHT22
const int DHTPin = 26;
DHT dht(DHTPin, DHTTYPE);

static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
static char pressure[7];
static char altitude[7];

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long currentTime1 = millis();
unsigned long currentTime2 = millis();
unsigned long currentTime3 = millis();

//IsNumeric Function
boolean isNumeric(String str) {
  unsigned int stringLength = str.length();

  if (stringLength == 0) {
    return false;
  }

  boolean seenDecimal = false;

  for (unsigned int i = 0; i < stringLength; ++i) {
    if (isDigit(str.charAt(i))) {
      continue;
    }

    if (str.charAt(i) == '.') {
      if (seenDecimal) {
        return false;
      }
      seenDecimal = true;
      continue;
    }
    return false;
  }
  return true;
}


void setup() {

  uint16_t width = 0;
  display.init();
  display.flipScreenVertically();

  dw_font_init(&myfont,
               128,
               64,
               draw_pixel,
               clear_pixel);

  //dht.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(pump, OUTPUT);
  digitalWrite(pump, LOW);
  pinMode(ldr, INPUT);
  pinMode(button, INPUT);

  ledcSetup(ledcchannel, freq, Res);
  ledcAttachPin(LED_PIN2 , ledcchannel);


  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for Internet time");

  while (!time(nullptr)) {
    Serial.print("*");
    delay(1000);
  }
  Serial.println("\nTime response....OK");

  Serial.println();

  Serial.println("start...");
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to initialise EEPROM");
    delay(1000);
  }

  // Uncomment the next lines to test the values stored in the flash memory
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print(byte(EEPROM.read(i)));
    Serial.print(" ");
  }

  /*  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;);
    }
    display.clearDisplay();
    display.setTextColor(WHITE);*/

  bool status;
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

}

void loop() {

  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("/esp/red");
      client.subscribe("/esp/yellow");
      client.subscribe("/node/pump");
      client.subscribe("/start/hour");
      client.subscribe("/start/minute");
      client.subscribe("/end/hour");
      client.subscribe("/end/minute");      
      client.subscribe("/pumpst/hour");
      client.subscribe("/pumpst/minute");
      client.subscribe("/pumpen/hour");
      client.subscribe("/pumpen/minute");
      client.subscribe("/oled/text");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      return;
    }
  }
  client.loop();
  Timezone();
  bmevalue();
  ldrval();
  buttonFunc();

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  int i = 0;

  while (i < length) msg += (char)payload[i++];

  //Serial.println(msg);

  if (msg == "Lightstate") {
    client.publish("/esp/red", (digitalRead(LED_PIN) ? "Light state ON" : "Light state OFF"));
    Serial.println("Send to check LED state!");
    return;
  }
  if (msg == "on") {
    client.publish("/esp/red", "Light State-ON");
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Send on!");
    return;
  }
  if (msg == "off") {
    client.publish("/esp/red", "Light State-OFF");
    digitalWrite(LED_PIN, LOW);
    Serial.println("Send off!");
    return;
  }

  if (msg == "state") {
    client.publish("/node/pump", (digitalRead(pump) ? "Pump state ON" : "Pump state OFF"));
    Serial.println("Send to check pump state!");
    return;
  }

  if (msg == "sendon") {
    client.publish("/node/pump", "Pump State-ON");
    digitalWrite(pump, HIGH);
    Serial.println("Send on!");
    return;
  }

  if (msg == "sendoff") {
    client.publish("/node/pump", "Pump State-OFF");
    digitalWrite(pump, LOW);
    Serial.println("Send off!");
    return;
  }

  if (isNumeric(msg)) {
    int x = msg.toInt();
    ledcWrite(ledcchannel , x);
  }

  //Set time to control light panel
  if (not strcmp(topic, "/start/hour")) {
    int sHour = msg.toInt();
    Serial.print("Start Hour =  ");
    Serial.println(sHour);
    EEPROM.write(0, sHour);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/start/minute")) {
    int sMinute = msg.toInt();
    Serial.print("Start Minute =  ");
    Serial.println(sMinute);
    EEPROM.write(1, sMinute);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/end/hour")) {
    int eHour = msg.toInt();
    Serial.print("End Hour =  ");
    Serial.println(eHour);
    EEPROM.write(2, eHour);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/end/minute")) {
    int eMinute = msg.toInt();
    Serial.print("End Minute =  ");
    Serial.println(eMinute);
    EEPROM.write(3, eMinute);
    EEPROM.commit();
  }

  //Set time to control pump
  if (not strcmp(topic, "/pumpst/hour")) {
    int pumpsHour = msg.toInt();
    Serial.print("Pump Start Hour =  ");
    Serial.println(pumpsHour);
    EEPROM.write(4, pumpsHour);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/pumpst/minute")) {
    int pumpsMinute = msg.toInt();
    Serial.print("Pump Start Minute =  ");
    Serial.println(pumpsMinute);
    EEPROM.write(5, pumpsMinute);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/pumpen/hour")) {
    int pumpeHour = msg.toInt();
    Serial.print("Pump End Hour =  ");
    Serial.println(pumpeHour);
    EEPROM.write(6, pumpeHour);
    EEPROM.commit();
  }

  if (not strcmp(topic, "/pumpen/minute")) {
    int pumpeMinute = msg.toInt();
    Serial.print("Pump End Minute =  ");
    Serial.println(pumpeMinute);
    EEPROM.write(7, pumpeMinute);
    EEPROM.commit();
  }

  /*if (not strcmp(topic, "/oled/text")) {
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(msg);
    display.display();
    }*/

  if (not strcmp(topic, "/oled/text")) {
     display.clear();
    dw_font_setfont(&myfont, &font_th_sarabunpsk_regular40);
    dw_font_goto(&myfont, 12, 40);
    dw_font_print(&myfont, (char*) msg.c_str());
    display.display();
  }

}

/*void dhtread() {

  if (millis() - currentTime1 > 3000) {
    currentTime1 = millis();

    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      strcpy(celsiusTemp, "Failed");
      strcpy(fahrenheitTemp, "Failed");
      strcpy(humidityTemp, "Failed");
    }
    else {
      // Computes temperature values in Celsius + Fahrenheit and Humidity
      float hic = dht.computeHeatIndex(t, h, false);
      dtostrf(hic, 6, 2, celsiusTemp);
      float hif = dht.computeHeatIndex(f, h);
      dtostrf(hif, 6, 2, fahrenheitTemp);
      dtostrf(h, 6, 2, humidityTemp);
      // You can delete the following Serial.prints, it s just for debugging purposes
    }

    client.publish("room/temperature", celsiusTemp);
    client.publish("room/fahrenheit", fahrenheitTemp);
    client.publish("room/humidity", humidityTemp);

  }

  }*/

void Timezone() {

  if (millis() - currentTime2 > 1000) {
    currentTime2 = millis();

    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
    int hour = p_tm->tm_hour;
    int minute = p_tm->tm_min;
    int sec = p_tm->tm_sec;
    //Serial.print(p_tm->tm_hour); Serial.print(":"); Serial.print(p_tm->tm_min); Serial.print(":"); Serial.println(p_tm->tm_sec);


    //Receive time input from User for control light (Store in EEPROM channel 0-3) 
    if (EEPROM.read(0) == hour && EEPROM.read(1) == minute) {
      Serial.println("sucess");
      Serial.println("");
      digitalWrite(LED_PIN, HIGH);
    }


    if (EEPROM.read(2) == hour && EEPROM.read(3) == minute) {
      Serial.println("stop");
      Serial.println("");
      digitalWrite(LED_PIN, LOW);
    }

    //Receive time input from User for control light (Store in EEPROM channel 0-3) 
    if (EEPROM.read(4) == hour && EEPROM.read(5) == minute) {
      Serial.println("pump ON");
      Serial.println("");
      digitalWrite(pump, HIGH);
    }


    if (EEPROM.read(6) == hour && EEPROM.read(7) == minute) {
      Serial.println("pump OFF");
      Serial.println("");
      digitalWrite(pump, LOW);
    }

  }
}

void bmevalue() {

  if (millis() - currentTime1 > 2500) {
    currentTime1 = millis();

    float celcius = bme.readTemperature();
    dtostrf(celcius, 6, 2, celsiusTemp);
    Serial.print("TempCelcius is  ");
    Serial.println(celsiusTemp);

    float fahrenheit = (1.8 * bme.readTemperature() + 32);
    dtostrf(fahrenheit, 6, 2, fahrenheitTemp);
    Serial.print("Tempfahrenheit is  ");
    Serial.println(fahrenheitTemp);

    float pressureF = (bme.readPressure() / 100.0F);
    dtostrf(pressureF, 6, 2, pressure);
    Serial.print("Pressure is  ");
    Serial.println(pressure);

    float altitudeF = (bme.readAltitude(SEALEVELPRESSURE_HPA));
    dtostrf(altitudeF, 6, 2, altitude);\
    Serial.print("Altitude is  ");
    Serial.println(altitude);

    float humidityF = bme.readHumidity();
    dtostrf(humidityF, 6, 2, humidityTemp);
    Serial.print("Humidity is  ");
    Serial.println(humidityTemp);

    client.publish("room/temperature", celsiusTemp);
    client.publish("room/fahrenheit", fahrenheitTemp);
    client.publish("room/humidity", humidityTemp);
    client.publish("test/pressure", pressure);
    client.publish("test/altitude", altitude);

  }
}

void ldrval() {

  if (millis() - currentTime3 > 2500) {
    currentTime3 = millis();

    int val = analogRead(ldr);
    String value =  String(val);
    //dtostrf(val, 6, 2, ldrvalue);
    client.publish("ldr/value", (char*) value.c_str());
  }

}

void buttonFunc() {

  int newstate = digitalRead(button);

  if (startstate != newstate) {
    Serial.println(newstate);
    client.publish("button/state", (newstate ? "Power ON" : "Power OFF"));
    startstate = newstate;
  }

}
