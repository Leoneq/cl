#include <Arduino.h>
#include <DNSServer.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include "time.h"

#define I2C_SDA 47
#define I2C_SCL 48

uint8_t curDig = 0;
uint8_t digDelay = 0;
uint8_t brightness = 100;
uint32_t dispValue = 0;
uint32_t newValue = 0;

const char *APssid = "VfdClock";

hw_timer_t *timer = NULL;
RTC_DS3231 rtc;
TwoWire rtci2c = TwoWire(0);
AsyncWebServer server(80);

uint8_t dispValueTable[6] = {1, 2, 3, 4, 5, 6};

const uint16_t digits[11] = {
    0b1111111111111101, // 0
    0b1111101111111111, // 1
    0b1111110111111111, // 2
    0b1111111011111111, // 3
    0b1111111101111111, // 4
    0b1111111110111111, // 5
    0b1111111111011111, // 6
    0b1111111111101111, // 7
    0b1111111111110111, // 8
    0b1111111111111011, // 9
    0b1111111111111111  // Blank
};

const uint8_t BUT_PIN[3] = {35, 36, 37};
const uint8_t HVEN_PIN = 38;
const uint8_t CLK_PIN = 39;
const uint8_t BLANK_PIN = 40;
const uint8_t STROBE_PIN = 41;
const uint8_t DATA_PIN = 42;

void updateTable()
{
    dispValue = newValue;
    dispValueTable[0] = dispValue % 10;
    dispValueTable[1] = (dispValue / 10) % 10;
    dispValueTable[2] = (dispValue / 100) % 10;
    dispValueTable[3] = (dispValue / 1000) % 10;
    dispValueTable[4] = (dispValue / 10000) % 10;
    dispValueTable[5] = (dispValue / 100000) % 10;
}
void shiftOut12(uint16_t val)
{
    for (int i = 0; i < 12; i++)
    {
        digitalWrite(DATA_PIN, ((val >> i) & 1));
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
    }
}

void setValue(bool isBlank)
{
    updateTable();
    digitalWrite(STROBE_PIN, LOW);
    for (int i = 0; i < 6; i++)
        shiftOut12(isBlank ? 10 : digits[dispValueTable[i]]);
    digitalWrite(STROBE_PIN, HIGH);
}

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
    </script>
    </body>
    </html>
    )rawliteral";

void setup()
{
    Serial.begin(115200);
    for (int i = 38; i < 43; i++)
        pinMode(i, OUTPUT);
    for (int i = 0; i < 3; i++)
        pinMode(BUT_PIN[i], INPUT);
    digitalWrite(HVEN_PIN, LOW);
    digitalWrite(BLANK_PIN, 0);
    setValue(0);
    rtci2c.begin(I2C_SDA, I2C_SCL, 100000);
    rtc.begin(&rtci2c);
    EEPROM.begin(1);
    if (digitalRead(BUT_PIN[1]) == LOW)
    {
        WiFi.softAP(APssid);
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send_P(200, "text/html", index_html); });

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
}

void loop()
{
    DateTime time = rtc.now();
    newValue = time.second();
    newValue += time.minute() * 100;
    newValue += time.hour() * 10000;
    if (newValue != dispValue)
    {
        setValue(0);
    }
}