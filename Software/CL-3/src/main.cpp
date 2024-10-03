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

uint8_t buttonState = 0;
uint8_t mode = 0;
uint8_t curDig = 0;
uint8_t digDelay = 0;
uint8_t brightness = 100;
uint32_t dispValue = 0;
uint32_t newValue = 0;
uint32_t lastValue = 0;

unsigned long clickTime = 0;
unsigned long lastChangeTime = 0;

const char *APssid = "VfdClock";

hw_timer_t *timer = NULL;
RTC_DS3231 rtc;
TwoWire rtci2c = TwoWire(0);
AsyncWebServer server(80);

uint8_t dispValueTable[6] = {1, 2, 3, 4, 5, 6};
DateTime newTime;
uint8_t delta = 0;
uint8_t offTime[6] = {0, 0, 0};
uint8_t onTime[6] = {0, 0, 0};

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

void IRAM_ATTR leftInt()
{
    if ((digitalRead(BUT_PIN[0]) == LOW) && (buttonState == 0))
    {
        buttonState = 7;
        clickTime = millis();
    }
    else if ((digitalRead(BUT_PIN[0]) == HIGH) && (buttonState == 7) && (50 < (millis() - clickTime)) && ((millis() - clickTime) < 700))
    {
        buttonState = 1;
        clickTime = 0;
    }
    else if ((digitalRead(BUT_PIN[0]) == HIGH) && (buttonState == 7) && ((millis() - clickTime) >= 700))
    {
        buttonState = 2;
        clickTime = 0;
    }
}

void IRAM_ATTR middleInt()
{
    if ((digitalRead(BUT_PIN[1]) == LOW) && (buttonState == 0))
    {
        buttonState = 8;
        clickTime = millis();
    }
    else if ((digitalRead(BUT_PIN[1]) == HIGH) && (buttonState == 8) && (50 < (millis() - clickTime)) && ((millis() - clickTime) < 700))
    {
        buttonState = 3;
        clickTime = 0;
    }
    else if ((digitalRead(BUT_PIN[1]) == HIGH) && (buttonState == 8) && ((millis() - clickTime) >= 700))
    {
        buttonState = 4;
        clickTime = 0;
    }
}

void IRAM_ATTR rightInt()
{
    if ((digitalRead(BUT_PIN[2]) == LOW) && (buttonState == 0))
    {
        buttonState = 9;
        clickTime = millis();
    }
    else if ((digitalRead(BUT_PIN[2]) == HIGH) && (buttonState == 9) && (50 < (millis() - clickTime)) && ((millis() - clickTime) < 700))
    {
        buttonState = 5;
        clickTime = 0;
    }
    else if ((digitalRead(BUT_PIN[2]) == HIGH) && (buttonState == 9) && ((millis() - clickTime) >= 700))
    {
        buttonState = 6;
        clickTime = 0;
    }
}

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
    if (mode == 0)
    {
        digitalWrite(DATA_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(DATA_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
    }
    else if (mode == 1)
    {
        digitalWrite(DATA_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(DATA_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
    }
    else if (mode == 2)
    {
        digitalWrite(DATA_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(DATA_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
    }
    else if (mode == 3)
    {
        digitalWrite(DATA_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(DATA_PIN, LOW);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK_PIN, LOW);
        delayMicroseconds(1);
    }
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
    EEPROM.begin(8);
    for (int i = 1; i < 4; i++)
    {
        onTime[i - 1] = EEPROM.read(i);
    }
    for (int i = 4; i < 7; i++)
    {
        offTime[i - 4] = EEPROM.read(i);
    }

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
    attachInterrupt(BUT_PIN[0], leftInt, CHANGE);
    attachInterrupt(BUT_PIN[1], middleInt, CHANGE);
    attachInterrupt(BUT_PIN[2], rightInt, CHANGE);
}

void loop()
{
    DateTime time = rtc.now();

    if (mode == 0)
    {
        uint32_t OnTime, OffTime;
        newValue = time.second();
        newValue += time.minute() * 100;
        newValue += time.hour() * 10000;
        OnTime = onTime[2];
        OnTime += onTime[1] * 100;
        OnTime += onTime[0] * 10000;
        OffTime = offTime[2];
        OffTime += offTime[1] * 100;
        OffTime += offTime[0] * 10000;
        if (OffTime > OnTime)
        {
            if (OnTime <= newValue && OffTime > newValue)
                digitalWrite(HVEN_PIN, LOW);
            else
                digitalWrite(HVEN_PIN, HIGH);
        }
        else if (OffTime < OnTime)
        {
            if (OnTime > newValue && OffTime <= newValue)
                digitalWrite(HVEN_PIN, HIGH);
            else
                digitalWrite(HVEN_PIN, LOW);
        }
        else
        {
            digitalWrite(HVEN_PIN, LOW);
        }
    }
    else
    {
        digitalWrite(HVEN_PIN, LOW);
        newValue = newTime.second();
        newValue += newTime.minute() * 100;
        newValue += newTime.hour() * 10000;
    }

    if (buttonState != 0 && buttonState != 1 && buttonState != 5 && buttonState != 3 && buttonState < 7)
    {
        if (lastValue != newValue && mode == 1)
        {
            rtc.adjust(newTime);
        }
        if (mode == 2)
        {
            onTime[0] = newTime.hour();
            onTime[1] = newTime.minute();
            onTime[2] = newTime.second();
            for (int i = 1; i < 4; i++)
            {
                EEPROM.write(i, onTime[i - 1]);
            }
            EEPROM.commit();
        }
        if (mode == 3)
        {
            offTime[0] = newTime.hour();
            offTime[1] = newTime.minute();
            offTime[2] = newTime.second();
            for (int i = 4; i < 7; i++)
            {
                EEPROM.write(i, offTime[i - 4]);
            }
            EEPROM.commit();
        }
    }

    if (buttonState == 4)
    {
        curDig = 0;
        mode = (mode == 0) ? 1 : 0;
    }
    else if (buttonState == 3)
    {

        curDig = (curDig + 1) % 3;
    }
    else if (buttonState == 2)
    {

        curDig = 0;
        mode = (mode == 3) ? 1 : mode + 1;
    }
    else if (buttonState == 6)
    {

        curDig = 0;
        mode = (mode == 1) ? 3 : mode - 1;
    }
    else if (buttonState == 1)
    {
        newTime = DateTime(newTime.year(), newTime.month(), newTime.day(), curDig == 2 ? (newTime.hour() + 1) % 24 : newTime.hour(), curDig == 1 ? (newTime.minute() + 1) % 60 : newTime.minute(), curDig == 0 ? (newTime.second() + 1) % 60 : newTime.second());
    }
    else if (buttonState == 5)
    {
        newTime = DateTime(newTime.year(), newTime.month(), newTime.day(), curDig == 2 ? (newTime.hour() + 23) % 24 : newTime.hour(), curDig == 1 ? (newTime.minute() + 59) % 60 : newTime.minute(), curDig == 0 ? (newTime.second() + 59) % 60 : newTime.second());
    }

    if (buttonState != 0 && buttonState != 1 && buttonState != 5 && buttonState < 7)
    {
        setValue(0);
        if (mode == 1)
        {
            if (buttonState != 3)
                newTime = rtc.now();
                lastValue = newValue;
        }
        else if (mode == 2)
        {
            if (buttonState != 3)
            {
                newTime = rtc.now();
                newTime = DateTime(newTime.year(), newTime.month(), newTime.day(), onTime[0], onTime[1], onTime[2]);
            }
        }
        else if (mode == 3)
        {
            if (buttonState != 3)
            {
                newTime = rtc.now();
                newTime = DateTime(newTime.year(), newTime.month(), newTime.day(), offTime[0], offTime[1], offTime[2]);
            }
        }
    }

    if (buttonState < 7)
    {
        buttonState = 0;
    }

    if (newValue != dispValue)
    {
        setValue(0);
    }
}