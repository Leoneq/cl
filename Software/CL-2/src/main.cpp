#include <Arduino.h>
#include <DNSServer.h>
#include <driver/adc.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include "time.h"

#define PWM_CHANNEL 0
#define PWM_FREQ 45000
#define PWM_RESOLUTION 8
#define PWM_PIN 8
#define I2C_SDA 47
#define I2C_SCL 48

uint8_t curDig = 0;
uint8_t digDelay = 0;
uint8_t brightness = 100;
uint16_t dispValue = 0;
uint16_t newValue = 0;
uint16_t pwmValue = 0;
uint16_t analogValue = 0;
float voltageValue = 0;
const char *APssid = "VfdClock";

hw_timer_t *timer = NULL;
RTC_DS3231 rtc;
TwoWire rtci2c = TwoWire(0);
AsyncWebServer server(80);

uint8_t dispValueTable[4] = {1, 2, 3, 4};
uint8_t dots = 0;

const uint8_t digits[11] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000  // Blank
};

const uint8_t SEG_PIN[8] = {13, 12, 10, 17, 14, 16, 15, 11};
const uint8_t DIG_PIN[4] = {35, 36, 37, 7};
const uint8_t BUT_PIN[3] = {4, 5, 6};

void setDigit(uint8_t digit)
{
    for (int i = 0; i < 7; i++)
        digitalWrite(SEG_PIN[i], (digits[digit] >> i) & 1 ? HIGH : LOW);
}

void updateTable()
{
    dispValue = newValue;
    dispValueTable[0] = dispValue % 10;
    dispValueTable[1] = (dispValue / 10) % 10;
    dispValueTable[2] = (dispValue / 100) % 10;
    dispValueTable[3] = (dispValue / 1000) % 10;
}

void IRAM_ATTR onTimer()
{
    if (digDelay == 5)
    {
        setDigit(dispValueTable[curDig]);
        digitalWrite(curDig, dots >> curDig);
    }
    else if (digDelay == (126 - brightness))
    {
        digitalWrite(DIG_PIN[curDig], HIGH);
    }

    if (digDelay >= 125)
    {
        digitalWrite(DIG_PIN[curDig], LOW);
        digitalWrite(dots >> curDig, LOW);
        digDelay = 0;
        curDig = (curDig + 1) % 4;
    }
    else
    {
        digDelay++;
    }
}

const int output = 2;
int maxVoltage = 40;
String sliderValue = "0";

const char *PARAM_INPUT = "value";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>Vfd Clock</h2>
  <p>Voltage:<span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="45" value="%SLIDERVALUE%" step="1" class="slider"></p>
  <button onclick="dateSync(this)">Sync TIme</button>
<script>
function dateSync(element) {
var date = new Date();
var current_time = date.getFullYear()+":"+(date.getMonth()+1)+":"+ date.getDate()+":"+date.getHours()+":"+date.getMinutes()+":"+ date.getSeconds();
console.log(date);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/date?value="+current_time, true);
  xhr.send();
  }
function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String processor(const String &var)
{
    // Serial.println(var);
    if (var == "SLIDERVALUE")
    {
        return sliderValue;
    }
    return String();
}

void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < 4; i++)
        pinMode(DIG_PIN[i], OUTPUT);
    for (int i = 0; i < 8; i++)
        pinMode(SEG_PIN[i], OUTPUT);
    for (int i = 0; i < 3; i++)
        pinMode(BUT_PIN[i], INPUT);
    rtci2c.begin(I2C_SDA, I2C_SCL, 100000);
    rtc.begin(&rtci2c);
    EEPROM.begin(1);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    timer = timerBegin(0, 19, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 40, true);
    timerAlarmEnable(timer);
    DateTime time = rtc.now();
    newValue = time.minute();
    newValue += time.hour() * 100;
    updateTable();
    if (digitalRead(BUT_PIN[1]) == LOW)
    {
        WiFi.softAP(APssid);
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send_P(200, "text/html", index_html, processor); });

        // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
        server.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            String inputMessage;
            // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
            if (request->hasParam(PARAM_INPUT))
            {
                inputMessage = request->getParam(PARAM_INPUT)->value();
                sliderValue = inputMessage;
              maxVoltage = sliderValue.toInt();
                EEPROM.write(0, maxVoltage);
                EEPROM.commit();
            }
            else
            {
                inputMessage = "No message sent";
            }
            request->send(200, "text/plain", "OK"); });

        server.on("/date", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            String inputMessage;
            if (request->hasParam(PARAM_INPUT))
            {
                inputMessage = request->getParam(PARAM_INPUT)->value();
                int years, months, days, hours, minutes, seconds;
                int   ArrayLength  =inputMessage.length()+1;
                char  CharArray[ArrayLength];
                inputMessage.toCharArray(CharArray,ArrayLength);
                sscanf(CharArray, "%d:%d:%d:%d:%d:%d", &years, &months, &days, &hours, &minutes, &seconds);
                rtc.adjust(DateTime(years,months,days,hours,minutes,seconds));
            }
            else
            {
                inputMessage = "No message sent";
            }
            request->send(200, "text/plain", "OK"); });
        server.begin();
    }
    maxVoltage = EEPROM.read(0);
    sliderValue = String(maxVoltage);
}

void loop()
{
    // Poniższy fragment kodu obsługuje przetwornicę. WAŻNE, NIE DAWAĆ DELAY W KODZIE BO MOŻE, ale nie musi, MOŻE SIĘ COŚ ZEPSUĆ
    ledcWrite(PWM_CHANNEL, pwmValue);
    analogValue = adc1_get_raw(ADC1_CHANNEL_0);
    float voltageValue = analogValue * 0.026;
    if (maxVoltage < voltageValue)
    {
        if (pwmValue > 0)
            pwmValue--;
    }
    else
    {
        if (pwmValue < 255)
            pwmValue++;
    }
    DateTime time = rtc.now();
    newValue = time.minute();
    newValue += time.hour() * 100;
    if (newValue != dispValue)
    {
        updateTable();
    }
}