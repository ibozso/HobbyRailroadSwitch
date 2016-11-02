#include "RailroadSwitchPeripheries.h"

#include <Arduino.h>

#define C_ELAPSED_TIME_LONG          2000u
#define C_ELAPSED_TIME_SHORT         100u

typedef enum
{
  BUTTON_STATE_HIGH,
  BUTTON_STATE_LOW,
} tButtonState;

tButtonEvent CheckButtonEvent(int ButtonPin)
{
  static tButtonState ButtonState = BUTTON_STATE_HIGH;
  static long StartTime;
  long ElapsedTime;
  tButtonEvent Result = BUTTON_EVENT_IDLE;
  
  if (digitalRead(ButtonPin) == LOW)
  {
    if (ButtonState == BUTTON_STATE_HIGH)
    {
      Serial.println("CheckButtonEvent(): ButtonState = BUTTON_STATE_LOW");
      ButtonState = BUTTON_STATE_LOW;
      StartTime = millis();
    }
  }
  else
  {
    if (ButtonState == BUTTON_STATE_LOW)
    {
      ButtonState = BUTTON_STATE_HIGH;
      ElapsedTime = millis() - StartTime;
      Serial.println("CheckButtonEvent(): ButtonState = BUTTON_STATE_HIGH");
      Serial.print("CheckButtonEvent(): ElapsedTime = ");
      Serial.println(ElapsedTime);

      if (ElapsedTime > C_ELAPSED_TIME_LONG)
      {
        Serial.println("CheckButtonEvent(): ButtonEvent = BUTTON_EVENT_LONGCLICK");
        Result = BUTTON_EVENT_LONGCLICK;
      }
      else
      {
        if (ElapsedTime > C_ELAPSED_TIME_SHORT)
        {
          Serial.println("CheckButton(): ButtonEvent = BUTTON_EVENT_SHORTCLICK");
          Result = BUTTON_EVENT_SHORTCLICK;
        }
      }
    }
  }

  return Result;
}

