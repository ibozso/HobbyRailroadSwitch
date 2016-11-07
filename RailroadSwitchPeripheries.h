/*
 *
 */

#ifndef _RAILROAD_SWITCH_PERIPHERIES_H_
#define _RAILROAD_SWITCH_PERIPHERIES_H_

#define COLOR_BLACK                  0x00000000u
#define COLOR_RED                    0x00FF0000u
#define COLOR_GREEN                  0x0000FF00u
#define COLOR_BLUE                   0x000000FFu
#define COLOR_ORANGE                 0x00FF8000u
#define COLOR_CYAN                   0x0000FFFFu

typedef enum
{
  BUTTON_EVENT_IDLE,
  BUTTON_EVENT_SHORTCLICK,
  BUTTON_EVENT_LONGCLICK,
} tButtonEvent;

typedef enum 
{ 
  SWITCH_DIRECTION_LEFT,
  SWITCH_DIRECTION_RIGHT,
  SWITCH_DIRECTION_NEUTRAL,
  SWITCH_DIRECTION_NUMBER,
} tSwitchDirection; 

extern void InitSwitch(void);
extern void SwitchCommand(unsigned int Id, tSwitchDirection Direction);
extern void SwitchControl(void);
extern void InitNeoPixel(void);
extern void SetNeoPixel(bool Blink, unsigned int Color);
extern tButtonEvent ButtonMonitor(void);

#endif

