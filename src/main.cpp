#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
//==============================================================================
#define dw digitalWrite
#define dr digitalRead
#define COI 2
#define RS_TX 3
#define RS_RX 4
#define RELAY1 5
#define RELAY2 6
#define NUT1 7
#define NUT2 8
#define NUT3 8
#define LED1 10
#define LED2 11
#define ESP_RX 12
#define ESP_TX 13
//---------------------------------------
#define SCREEN_ADDRESS 0x3D
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
//==============================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial RS485Serial(RS_RX, RS_TX); // RX, TX
SoftwareSerial ESPSerial(ESP_RX, ESP_TX); // RX, TX
ModbusMaster node;
Adafruit_AHTX0 aht;
//==============================================================================
sensors_event_t humidity, temp;
uint8_t address[][6] = {"1Node", "2Node"};
float ph = -1.0, soilMoisture = -1.0, soilTemp = -1.0, EC = -1.0, n = -1.0, p = -1.0, k = -1.0;
unsigned long lastCoiHigh, lastSend, lastCheckSensor, lastPress1, lastPress2, lastPress3;
int buttonState1, buttonState2, buttonState3;
int lastButtonState1, lastButtonState2, lastButtonState3;
int modeScreen = 0;    // modeScreen hien thi man hinh oled
int mode = 0;          // thu cong hoac tu dong
int selectedRelay = 0; // relay dang duoc chon
//==============================================================================

#define FRAME_DELAY (42)
#define FRAME_WIDTH (48)
#define FRAME_HEIGHT (48)
#define FRAME_COUNT (sizeof(frames) / sizeof(frames[0]))
const byte PROGMEM frames[][288] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 12, 48, 0, 0, 0, 0, 12, 48, 0, 0, 0, 0, 12, 48, 0, 0, 0, 120, 12, 16, 30, 0, 0, 252, 24, 24, 63, 0, 1, 199, 120, 30, 227, 128, 3, 131, 224, 7, 193, 192, 1, 128, 128, 1, 1, 128, 1, 128, 0, 0, 1, 128, 0, 192, 0, 0, 3, 0, 0, 96, 0, 0, 6, 0, 0, 96, 0, 0, 14, 0, 0, 48, 0, 128, 12, 0, 0, 96, 15, 240, 6, 0, 0, 96, 28, 120, 6, 0, 0, 192, 48, 12, 3, 0, 31, 192, 96, 6, 3, 248, 62, 0, 96, 6, 0, 252, 48, 0, 64, 2, 0, 12, 48, 0, 192, 2, 0, 12, 48, 0, 64, 3, 0, 12, 48, 0, 64, 2, 0, 12, 63, 0, 96, 6, 0, 124, 31, 192, 112, 6, 3, 248, 0, 192, 48, 12, 7, 0, 0, 96, 30, 56, 6, 0, 0, 96, 15, 240, 6, 0, 0, 48, 1, 0, 12, 0, 0, 112, 0, 0, 6, 0, 0, 96, 0, 0, 6, 0, 0, 192, 0, 0, 3, 0, 1, 128, 0, 0, 1, 128, 3, 128, 128, 1, 1, 192, 1, 131, 224, 7, 193, 192, 0, 199, 120, 30, 99, 128, 0, 124, 24, 24, 63, 0, 0, 56, 8, 48, 30, 0, 0, 0, 12, 48, 0, 0, 0, 0, 12, 48, 0, 0, 0, 0, 12, 48, 0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 7, 224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};
//==============================================================================
void displayStartScreen()
{
  display.clearDisplay();
  display.drawBitmap(40, 8, frames[0], FRAME_WIDTH, FRAME_HEIGHT, 1);
  display.display();
  delay(FRAME_DELAY);
}
//==============================================================================
// read aht sensor and save to humidity and temp
void readAHT()
{
  aht.getEvent(&humidity, &temp);
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" *C");
}

//==============================================================================
// toggle led for x times y ms
void toggleLED(int led, int delayTime, int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(led, HIGH);
    delay(delayTime);
    digitalWrite(led, LOW);
    delay(delayTime);
  }
}
//==============================================================================
// read rs485
void readRS485()
{
  uint8_t result;
  result = node.readHoldingRegisters(0x06, 1);
  if (result == node.ku8MBSuccess)
  {
    ph = (float)node.receive() / 100.0;
    Serial.print(ph);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x15, 1);
  if (result == node.ku8MBSuccess)
  {
    EC = (float)node.receive();
    Serial.print(EC);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x1e, 1);
  if (result == node.ku8MBSuccess)
  {
    n = (float)node.receive();
    Serial.print(n);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x1f, 1);
  if (result == node.ku8MBSuccess)
  {
    p = (float)node.receive();
    Serial.print(p);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x20, 1);
  if (result == node.ku8MBSuccess)
  {
    k = (float)node.receive();
    Serial.print(k);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x12, 1);
  if (result == node.ku8MBSuccess)
  {
    soilMoisture = (float)node.receive() / 10.0;
    Serial.print(soilMoisture);
    Serial.print('\t');
  }
  result = node.readHoldingRegisters(0x13, 1);
  if (result == node.ku8MBSuccess)
  {
    k = (float)node.receive() / 10.0;
    Serial.print(k);
    Serial.print('\t');
  }
}
//==============================================================================
// send data to esp using only one string
void sendDataToESP()
{
  String data = String(ph) + "," + String(EC) + "," + String(n) + "," + String(p) + "," + String(k) + "," + String(soilMoisture) + "," + String(soilTemp) + "," + String(humidity.relative_humidity) + "," + String(temp.temperature);
  ESPSerial.println(data);
}
//==============================================================================
// update data on oled display
void updateDataOnDisplay(int piece)
{
  if (piece == 1)
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.println(temp.temperature);
    display.drawFastHLine(0, 8, display.width(), SSD1306_WHITE);
    display.setCursor(0, 10);
    display.print("Humi: ");
    display.println(humidity.relative_humidity);
    display.drawFastHLine(0, 18, display.width(), SSD1306_WHITE);
    display.setCursor(0, 20);
    display.print("ph: ");
    display.println(ph);
    display.drawFastHLine(0, 28, display.width(), SSD1306_WHITE);
    display.setCursor(0, 30);
    display.print("sHum: ");
    display.println(soilMoisture);
    display.drawFastHLine(0, 38, display.width(), SSD1306_WHITE);
    display.setCursor(0, 40);
    display.print("sTemp: ");
    display.println(soilTemp);
    display.drawFastHLine(0, 48, display.width(), SSD1306_WHITE);
    display.setCursor(0, 50);
  }
  else if (piece == 2)
  {
    display.drawFastVLine(display.width() / 2, 0, display.height(), SSD1306_WHITE);
    display.setCursor(70, 0);
    display.print("n: ");
    display.println(n);
    display.setCursor(70, 10);
    display.print("p: ");
    display.println(p);
    display.setCursor(70, 20);
    display.print("k: ");
    display.println(k);
    display.setCursor(70, 30);
    display.print("EC: ");
    display.println(EC);
    display.setCursor(70, 40);
  }
}
//==============================================================================
void setup()
{
  Serial.begin(115200);
  ESPSerial.begin(115200);
  Serial.println("Starting...");
  //---------------------------------------
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    // toggle led1 for 2 times 1000ms each
    toggleLED(LED1, 1000, 2);
  }
  display.display();
  delay(1000); // Pause for 2 seconds
  display.clearDisplay();
  displayStartScreen();
  delay(2000);
  display.clearDisplay();
  //---------------------------------------
  pinMode(COI, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(NUT1, INPUT_PULLUP);
  pinMode(NUT2, INPUT_PULLUP);
  pinMode(NUT3, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  //---------------------------------------
  if (aht.begin())
  {
    Serial.println("Found AHT20");
  }
  else
  {
    Serial.println("Didn't find AHT20");
    toggleLED(LED1, 1000, 3);
  }
  //---------------------------------------
  RS485Serial.begin(4800);
  node.begin(1, RS485Serial);
  //---------------------------------------
}

void loop()
{
  buttonState1 = digitalRead(NUT1);
  buttonState2 = digitalRead(NUT2);
  buttonState3 = digitalRead(NUT3);

  if (buttonState1 == LOW && lastButtonState1 == HIGH)
  {
    lastPress1 = millis();
  }
  if (buttonState2 == LOW && lastButtonState2 == HIGH)
  {
    lastPress2 = millis();
  }
  if (buttonState3 == LOW && lastButtonState3 == HIGH)
  {
    lastPress3 = millis();
  }

  // release button 1 for modeScreen change
  if (buttonState1 == HIGH && lastButtonState1 == LOW)
  {
    unsigned long elapsedTime = millis() - lastPress1;
    if (elapsedTime > 1000 && elapsedTime < 3000)
    {
      dw(COI, HIGH);
      lastCoiHigh = millis();
      if (modeScreen == 0)
        modeScreen = 1;
      else
        modeScreen = 0;
    }
    else if (elapsedTime > 3000)
    {
      dw(COI, HIGH);
      lastCoiHigh = millis();
      if (mode == 0)
        mode = 1;
      else
        mode = 0;
    }
  }
  // release button 2 for selected thing change
  if (buttonState2 == HIGH && lastButtonState2 == LOW)
  {
    unsigned long elapsedTime = millis() - lastPress2;
    if (elapsedTime > 1000 && elapsedTime < 3000)
    {
      dw(COI, HIGH);
      lastCoiHigh = millis();
      if (selectedRelay == 0)
        selectedRelay = 1;
      else
        selectedRelay = 0;
    }
  }
  // release button 3 for selected thing status change
  if (buttonState3 == HIGH && lastButtonState3 == LOW)
  {
    unsigned long elapsedTime = millis() - lastPress3;
    if (elapsedTime > 1000)
    {
      dw(COI, HIGH);
      lastCoiHigh = millis();
      if (selectedRelay == 0)
      {
        dw(RELAY1, !dr(RELAY1));
        dw(LED1, !dr(LED1));
      }
      else
      {
        dw(RELAY2, !dr(RELAY2));
        dw(LED2, !dr(LED2));
      }
    }
  }
  // DONE: add modeScreen 1 show info on oled
  if (modeScreen == 1)
  {
    display.clearDisplay();
    // show info of relay 1 and 2 in the oled, the left side is relay 1 and the right side is relay 2, each will hold 1/2 of the width of the oled and 2/3 of the height of the oled
    if (dr(RELAY1) == HIGH)
    {
      display.drawRect(0, 0, display.width() / 2 - 1, display.height() * 2 / 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }
    else
    {
      display.fillRect(0, 0, display.width() / 2 - 1, display.height() * 2 / 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(10, 20);
    display.print("Relay 1");
    if (dr(RELAY2) == HIGH)
    {
      display.drawRect(display.width() / 2 + 1, 0, display.width() / 2, display.height() * 2 / 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }
    else
    {
      display.fillRect(display.width() / 2 + 1, 0, display.width() / 2 - 1, display.height() * 2 / 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(72, 20);
    display.print("Relay 2");
    display.setCursor(29 + 62 * selectedRelay, display.height() * 2 / 3 + 1);
    display.setTextColor(SSD1306_WHITE);
    display.write(0x5e);
  }

  if (millis() - lastCoiHigh > 500 && dr(COI) == HIGH)
  {
    dw(COI, LOW);
  }
  if (millis() - lastSend > 3000)
  {
    sendDataToESP();
    lastSend = millis();
    // update infor on display
    display.setCursor(10, 57);
    display.fillRect(0, 56, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK); // Draw white text
    display.print("S")
  }
  if (millis() - lastCheckSensor > 3000)
  {
    display.setTextColor(SSD1306_WHITE); // Draw white text
    readAHT();
    readRS485();
    if (modeScreen == 0)
    {
      updateDataOnDisplay(1);
      updateDataOnDisplay(2);
    }
    lastCheckSensor = millis();
  }
  if (ESPSerial.available())
  {
    String data = ESPSerial.readStringUntil('\n');
    Serial.println(data);
    if (data == "r1")
    {
      dw(RELAY1, !dr(RELAY1));
      dw(LED1, !dr(LED1));
      dw(COI, HIGH);
      lastCoiHigh = millis();
    }
    if (data == "r2")
    {
      dw(RELAY2, !dr(RELAY2));
      dw(LED2, !dr(LED2));
      dw(COI, HIGH);
      lastCoiHigh = millis();
    }
    // update infor on display
    display.setCursor(1, 57);
    display.fillRect(0, 56, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK); // Draw white text
    display.print("R")
  }

  display.display();
}
