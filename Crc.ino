#include <Arduino.h>
#include "Crc.h"

/*
 * 
 */

unsigned char CalcCRC(unsigned char CRC, char *PC, unsigned int Len)
{
  unsigned int II;
  unsigned char IB;
  unsigned char CC;

  for (II = 0u; II < Len; II++, PC++)
  {
    CC = *PC;

    for (IB = 0u; IB < 8u; IB++)
    {
      CRC <<= 1u;
      
      if ((CC ^ CRC) & 0x80u)
      {
        CRC ^= 0x09u;
      }

      CC <<= 1u;
    }

    CRC &= 0x7Fu;
  }

  return CRC;
}

