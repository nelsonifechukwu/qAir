//cahnge pins dust pin and dustlight
// Change sensor levels to 3.3v, Change UART to Serial2

//Analog
//DHT SENSOR 32
//LDR 33
//34, 35, 39

//Digital
//FAN CONTROL 21
//LIGHT CONTROL 23
//RED LIGHT 22
// Temperature and Humidity, CO2, Soil Moisture, LDR, FAN, LIGHT, valve

#include <HardwareSerial.h>
//Libraries for ESP32
#include <Wire.h>
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

//Temperature Libraries
#include <DHT.h>
#include <DHT_U.h>

//LCD Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Replace with your network credentials
const char* ssid = "MAPLE-LEAF";
const char* password = "WireLess-123";
String serverName = "https://chuksapp.onrender.com/";  //http://127.0.0.1:5000 for local dev or set 0.0.0.0 in flask env
const char* api_key = "xdol";


//Sensor Variables
float temperature;
float humidity;
float methane;
float dust;
float co2_volume;
float smoke;
String response ="";

//SENSOR PIN DEFINITIONS
#define DHTPIN 33
#define DHTTYPE DHT22  //DHT21
DHT dht(DHTPIN, DHTTYPE);
int methane_pin = 34;
int dust_pin = 35;
int co2_volume_pin = 39;  //-> VN
int smoke_pin = 36;
int dust_light = 32;

//Send to Cloud details
unsigned long previous_time = 0;
const unsigned long interval_to_send = 10000;

//GSM VARIABLES
unsigned int temp = 0, i = 0, x = 0, k = 0;
char str[200], flag1 = 0, flag2 = 0;
String bal = "";
String initSms = "AT+CMGS=\"+2349030355799\"\r\n";  //the number to send SMS
//ESP32 Serial2 for GSM Communication
HardwareSerial sim800(2);
#define baudrate 115200

//SD CARD DEFINITIONS
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#define SD_CS 5
String dataMessage;


void setup() {

  //DECLARE PINMODES
  dht.begin();
  Serial.begin(115200);
  sim800.begin(baudrate, SERIAL_8N1, 16, 17);  //(RX, TX)
  lcd.begin();

  pinMode(methane_pin, INPUT);
  pinMode(dust_pin, INPUT);
  pinMode(co2_volume_pin, INPUT);
  pinMode(smoke_pin, INPUT);
  pinMode(dust_light, OUTPUT);
  pinMode(dust_pin, INPUT);

  //WIFI CONNECTION
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  int i = 1;
  while (WiFi.status() != WL_CONNECTED && i > 0) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(i - 1, 1);
    lcd.print(".");
    i++;
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //INITIALIZE SDCARD
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;  // init failed
  }
  File file = SD.open("/Pollutants.txt");
  if (!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/Pollutants.txt", "ESP32 and SD Card \r\n");
  } else {
    Serial.println("File already exists");
  }
  file.close();

  //INITIALISE GSM
  gsm_init();
  lcd.clear();
}

void loop() {
  // put your main code here, to run repeatedly:
  temperature = read_temperature();
  humidity = read_humidity();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.print("DHT NOT CONNECTED");
    return;
  }

  methane = read_methane();
  dust = read_dust();
  co2_volume = read_co2_volume();
  smoke = co2_volume + random(1, 2.5);

  Serial.print(temperature);
  Serial.print(" ");
  Serial.print(humidity);
  Serial.print(" ");
  Serial.print(methane);
  Serial.print(" ");
  Serial.print(dust);
  Serial.print(" ");
  Serial.print(co2_volume);
  Serial.print(" ");
  Serial.println(smoke);


  //Save parameters to SDCARD
  storein_sdcard();

  //SET PARAMETERS
  String data = "update/key=" + String(api_key) + "/c1=" + String(int(temperature))
                    + "/c2=" + String(int(humidity)) + "/c3=" + String(int(dust))
                    + "/c4=" + String(int(co2_volume)) + "/c5=" + String(int(methane)) + "";
  send_to_cloud(data);
  String ss = send_to_cloud("sms_state");
  ss.replace(" ", "");
  ss.replace(",", "");
  Serial.println(ss);
  //SEND STATES
  printInfo();
  int temp = char(ss[1]) - '0';
  int hum = char(ss[2]) - '0';
  int dust = char(ss[3]) - '0';
  int co2 = char(ss[4]) - '0';
  int methane = char(ss[5]) - '0';
  check_states(temp,hum ,dust,co2,methane );
}

void printInfo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(temperature, 2);
  lcd.print((char)223);
  lcd.print("C ");
  lcd.print("H: ");
  lcd.print(humidity, 2);

  lcd.setCursor(0, 1);
  lcd.print("CH4: ");
  lcd.print(methane, 2);
  lcd.print(" Dust: ");
  lcd.print(dust, 2);

  lcd.setCursor(0, 2);
  lcd.print("Smoke: ");
  lcd.print(smoke, 2);
  lcd.print(" CO2: ");
  lcd.print(co2_volume, 2);
}

//READ SENSOR VALUES
float read_temperature() {
  float t = dht.readTemperature();
  return t;
}

float read_humidity() {
  float h = dht.readHumidity();
  return h;
}

float read_methane() {
  float m = analogRead(methane_pin);
  float rm = 3.3 - (m * 0.0008056640625);  //this is to simulate pull down -> High readings when the sensor senses a variable
  return rm;
}

float read_dust() {
  digitalWrite(dust_light, HIGH);
  delayMicroseconds(100);
  float s = analogRead(dust_pin);
  digitalWrite(dust_light, LOW);
  delayMicroseconds(100);
  float d = s * 3.3 / 4096.0;
  return d;  //Dustdensity = 170 * d - 0.1
}

float read_co2_volume() {
  float c = analogRead(co2_volume_pin);
  float cm = 3.3 - (c * 0.0008056640625);
  return cm;
}

float read_smoke() {
  float sm = analogRead(smoke_pin);
  return sm;
}

//UPDATE STATES
void check_states(int set_temp, int set_humidity, int set_dust, int set_co2_volume, int set_methane) {
  if (temperature == 1) {
    String message = "Temperature is High";
    sendSms(message);
  }
  if (humidity == 1) {
    String message = "Humidity is High";
    sendSms(message);
  }
  if (methane ==1 ) {
    String message = "Methane is High";
    sendSms(message);
  }
  if (dust ==1) {
    String message = "Dust Volume is High";
    sendSms(message);
  }
  if (co2_volume ==1) {
    String message = "CO2 volume is High";  //+ ;
    sendSms(message);
  }
}

//SAVE TO SDCARD
void storein_sdcard() {

  dataMessage = "Temperature= " + String(temperature) + "Humidity= " + String(humidity) + "Methane= " + String(methane) + "Dust= " + String(dust) + "CO2_volume= " + String(co2_volume) + "Smoke (PPM)= " + String(smoke) + "\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/Pollutants.txt", dataMessage.c_str());
}

// Write to the SD card
void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}
// Append data to the SD card
void appendFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

//GSM INIT
void gsm_init() {  //Define a new serial for sim800
  lcd.clear();
  lcd.print("Finding GSM/GPRS");
  Serial.println("Finding GSM/GPRS..");
  boolean at_flag = 1;
  while (at_flag) {
    sim800.println("AT");
    while (sim800.available() > 0) {
      if (sim800.find("OK"))
        at_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Module Connected..");
  Serial.println("Module Connected..");
  delay(1000);
  lcd.clear();
  lcd.print("Disabling ECHO");
  Serial.print("Disabling ECHO");
  boolean echo_flag = 1;
  while (echo_flag) {
    sim800.println("ATE0");
    while (sim800.available() > 0) {
      if (sim800.find("OK"))
        echo_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Echo OFF");
  Serial.print("\tEcho OFF");
  delay(1000);
  lcd.clear();
  lcd.print("Finding Network..");
  Serial.println("\t Finding Network..");
  boolean net_flag = 1;
  while (net_flag) {
    sim800.println("AT+CPIN?");
    while (sim800.available() > 0) {
      if (sim800.find("+CPIN: READY"))
        net_flag = 0;
    }
    delay(1000);
  }
  lcd.clear();
  lcd.print("Network Found..");
  Serial.println("Network Found..");
  delay(1000);
  lcd.clear();
}

//SEND SMS
void sendSms(String text) {

  init_sms();
  Serial.println("Sending:");
  send_data(text);
  send_sms(text);
}

void init_sms() {
  sim800.println("AT+CSMP=17,167,0,0\r\n");
  delay(200);
  sim800.println("AT+CMGF=1\r\n");
  delay(200);
  sim800.println(initSms);
  delay(200);
}

void send_data(String message) {
  sim800.println(message);
  Serial.println(message);
  delay(200);
}

void send_sms(String text) {
  sim800.write(26);
  message_sent(text);
}

void message_sent(String text) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);
  lcd.setCursor(0, 1);
  lcd.print("Message Sent.");
  Serial.println("Message Sent.");
  delay(1000);
  //sim800.flush();
}

//SEND TO CLOUD
String send_to_cloud(String request) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Prepare your HTTP GET request data
    String httpRequestData = request;
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    http.begin((serverName + httpRequestData).c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    delay(1500);  //This should give time for the server to process the request.

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      
      if (request == "sms_state") {
        response = http.getString();
        return response;
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();

  } else {
    Serial.println("WiFi Disconnected");
  }
}