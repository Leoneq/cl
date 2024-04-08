#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

uint16_t pwmValue = 0;

void setup()
{
  ledcSetup(0, 500000, 12);
  ledcAttachPin(8,0);
  analogReadResolution(12);
}

void loop()
{
  // Poniższy fragment kodu obsługuje przetwornicę, wartość w ifie odpowiada Vout*14.465408805
  ledcWrite(0, pwmValue);
  if (analogRead(1) > 145)
  {
    pwmValue--;
  }
  else 
  {
    pwmValue++;
  }
}