/*
 * Arduino Climate Control & Security (Final Version with DEBUG)
 * DHT11 + OLED 128x32 + Active Buzzer + HC-06
 * Добавлена диагностика команд на экран
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <SoftwareSerial.h>

// --- Настройки дисплея ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Настройки Пинов ---
#define DHTPIN 2
#define DHTTYPE DHT11
#define BUZZER_PIN 3 

// --- Bluetooth ---
// ВАЖНО: HC-06 TX подключаем к Arduino Pin 10 (Прием команд)
// ВАЖНО: HC-06 RX подключаем к Arduino Pin 11 (Отправка данных)
SoftwareSerial BTSerial(10, 11); // RX, TX

DHT dht(DHTPIN, DHTTYPE);

// --- Логика ---
bool isAlarmSystemOn = false; // По умолчанию выключено
float tempThreshold = 24.0;  
float humThreshold = 75.0;   

// Переменные для таймера
unsigned long previousMillis = 0;
const long interval = 2000; 

// Для отображения отладки на экране
String lastCommandStatus = ""; 
unsigned long commandTimer = 0;

float h = 0;
float t = 0;

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 

  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("System Loading..."));
  display.display();
  delay(1000);
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Читаем Bluetooth (ВСЕГДА) ---
  if (BTSerial.available()) {
    char cmd = BTSerial.read();
    Serial.print("Cmd: "); 
    Serial.println(cmd);
    
    // Запускаем таймер показа команды на экране
    commandTimer = millis();
    
    if (cmd == '1') {
      isAlarmSystemOn = true;
      lastCommandStatus = "CMD: ON"; // Покажем это на экране
    } else if (cmd == '0') {
      isAlarmSystemOn = false;
      digitalWrite(BUZZER_PIN, LOW); 
      lastCommandStatus = "CMD: OFF"; // Покажем это на экране
    } else {
      lastCommandStatus = "CMD: ??";
    }
  }

  // --- 2. Логика сигнализации ---
  bool alarmTriggered = false;
  if (isAlarmSystemOn) {
    if (t > tempThreshold || h > humThreshold) {
      alarmTriggered = true;
      if (currentMillis % 200 < 100) { 
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // --- 3. Чтение датчика и Экран (Раз в 2 секунды или если пришла команда) ---
  // Мы обновляем экран чаще, если пришла команда, чтобы ты сразу увидел реакцию
  bool showCommandNow = (millis() - commandTimer < 2000 && lastCommandStatus != "");
  
  if (currentMillis - previousMillis >= interval || showCommandNow) {
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        float newH = dht.readHumidity();
        float newT = dht.readTemperature();
        if (!isnan(newH) && !isnan(newT)) {
          h = newH;
          t = newT;
          // Отправка на телефон только по таймеру
          String data = "T:" + String(t) + ";H:" + String(h) + ";";
          BTSerial.println(data);
        }
    }

    // --- Рисуем Экран ---
    display.clearDisplay();
    
    display.setTextSize(1);
    display.setCursor(0,0);
    
    // Если только что нажали кнопку - показываем команду
    if (showCommandNow) {
        display.print(lastCommandStatus); 
    } else {
        // Иначе показываем статус охраны
        if (isAlarmSystemOn) display.print("ALARM: ON");
        else display.print("ALARM: OFF");
    }

    if (alarmTriggered) {
      display.setCursor(80, 0);
      display.print("!ALERT!");
    }

    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print((int)t);
    display.print("C  ");
    display.print((int)h);
    display.print("%");
    
    display.display();
  }
}