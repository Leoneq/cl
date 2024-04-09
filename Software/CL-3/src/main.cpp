#include <Arduino.h>
#include <driver/adc.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define PWM_CHANNEL 0
#define PWM_FREQ 40000
#define PWM_RESOLUTION 7
#define PWM_PIN 8
#define I2C_SDA 47
#define I2C_SCL 48

uint8_t curDig = 0;
uint8_t digDelay = 0;
uint8_t brightness = 100;
uint32_t dispValue = 1;
uint32_t newValue = 000000;
uint16_t pwmValue = 0;
uint16_t analogValue = 0;
float voltageValue = 0;

const uint8_t COMA_PIN[3] = {42, 4, 13};
const uint8_t NUM_PIN[10] = {41, 5, 6, 7, 15, 16, 17, 38, 39, 40};
const uint8_t DIG_PIN[6] = {3, 46, 9, 10, 11, 12};

uint8_t dispValueTable[6] = {1, 2, 3, 4, 5, 6};

RTC_DS3231 rtc;
TwoWire rtci2c = TwoWire(0);

void setValue()
{
    for (int i = 0; i < 10; i++)
        digitalWrite(NUM_PIN[i], LOW);
    for (int i = 0; i < 6; i++)
    {
        // digitalWrite(NUM_PIN[dispValueTable[i]], LOW);
        // delayMicroseconds(10);
        digitalWrite(DIG_PIN[i], HIGH);
        delayMicroseconds(10);
        digitalWrite(DIG_PIN[i], LOW);
        // delayMicroseconds(10);
        // digitalWrite(NUM_PIN[dispValueTable[i]], LOW);
    }
    digitalWrite(NUM_PIN[3], HIGH);
    digitalWrite(DIG_PIN[3], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[3], LOW);
    digitalWrite(NUM_PIN[3], LOW);

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

void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < 3; i++)
        pinMode(COMA_PIN[i], OUTPUT);
    for (int i = 0; i < 10; i++)
        pinMode(NUM_PIN[i], OUTPUT);
    for (int i = 0; i < 6; i++)
        pinMode(DIG_PIN[i], OUTPUT);
    rtci2c.begin(I2C_SDA, I2C_SCL, 100000);
    rtc.begin(&rtci2c);
    EEPROM.begin(1);
    if (EEPROM.read(0) != 7)
    {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        EEPROM.write(0, 7);
    }
    EEPROM.end();
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    ledcWrite(PWM_CHANNEL, pwmValue);
    for (int i = 0; i < 10; i++)
        digitalWrite(NUM_PIN[i], LOW);
    for (int i = 0; i < 6; i++)
    {
        // digitalWrite(NUM_PIN[dispValueTable[i]], LOW);
        // delayMicroseconds(10);
        digitalWrite(DIG_PIN[i], HIGH);
        delayMicroseconds(10);
        digitalWrite(DIG_PIN[i], LOW);
        // delayMicroseconds(10);
        // digitalWrite(NUM_PIN[dispValueTable[i]], LOW);
    }

}
float highestVoltage = 0;
uint32_t bestFreq = 0;
void loop()
{
    // Poniższy fragment kodu obsługuje przetwornicę. WAŻNE, NIE DAWAĆ DELAY W KODZIE BO MOŻE, ale nie musi, MOŻE SIĘ COŚ ZEPSUĆ
    ledcWrite(PWM_CHANNEL, pwmValue);
    analogValue = adc1_get_raw(ADC1_CHANNEL_0);
    float voltageValue = analogValue * 0.04815;
    pwmValue=125;
    // if (170 < voltageValue)
    // {
    //     if (pwmValue > 0)
    //         pwmValue--;
    // }
    // else
    // {
    //     if (pwmValue < 125)
    //         pwmValue++;
    // }
    // if (highestVoltage < voltageValue)
    //{
    //      bestFreq = ledcReadFreq(PWM_CHANNEL);
    //      highestVoltage = voltageValue;
    //  }
    //  delay(300);
    Serial.println("Analog Value: " + String(analogValue) + " Voltage Value: " + String(voltageValue) + " PWM Value " + String(pwmValue));

    // if (pwmValue == 60)
    // {
    //     pwmValue = 0;
    //     ledcChangeFrequency(PWM_CHANNEL, ledcReadFreq(PWM_CHANNEL) + 5000, PWM_RESOLUTION);
    // }

    DateTime time = rtc.now();
    digitalWrite(NUM_PIN[3], HIGH);
    digitalWrite(DIG_PIN[3], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[3], LOW);
    digitalWrite(NUM_PIN[3], LOW);
    delay(2);
    digitalWrite(DIG_PIN[3], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[3], LOW);

    digitalWrite(NUM_PIN[2], HIGH);
    digitalWrite(DIG_PIN[2], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[2], LOW);
    digitalWrite(NUM_PIN[2], LOW);
    delay(2);
    digitalWrite(DIG_PIN[2], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[2], LOW);

    digitalWrite(NUM_PIN[8], HIGH);
    digitalWrite(DIG_PIN[6], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[6], LOW);
    digitalWrite(NUM_PIN[8], LOW);
    delay(2);
    digitalWrite(DIG_PIN[6], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[6], LOW);

    digitalWrite(NUM_PIN[7], HIGH);
    digitalWrite(DIG_PIN[4], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[4], LOW);
    digitalWrite(NUM_PIN[7], LOW);
    delay(2);
    digitalWrite(DIG_PIN[4], HIGH);
    delayMicroseconds(10);
    digitalWrite(DIG_PIN[4], LOW);
    
}