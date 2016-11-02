#ifndef _RAILROAD_SWITCH_PERIPHERIES_H_
#define _RAILROAD_SWITCH_PERIPHERIES_H_

typedef enum
{
  BUTTON_EVENT_IDLE,
  BUTTON_EVENT_SHORTCLICK,
  BUTTON_EVENT_LONGCLICK,
} tButtonEvent;

extern tButtonEvent CheckButtonEvent(int ButtonPin);

#endif

