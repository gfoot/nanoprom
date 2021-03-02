#include <Arduino.h>

#include "eeprom.h"

#include "configs.h"


void eeprom_init()
{
  /* Set all pins to output high */
  PORTD |= 0xfc;
  DDRD |= 0xfc;
  PORTB |= 0x3f;
  DDRB |= 0x3f;
  PORTC |= 0x3f;
  DDRC |= 0x3f;
}


static void writeByte(uint16_t address, uint8_t data)
{
	PORTD = (PORTD & 0x03) | ((address & 0x38) >> 1) | ((data & 0xc0) >> 1) | 0x80;
  PORTB = (PORTB & 0xc0) | ((address & 0xc00) >> 10) | ((address & 0x200) >> 7) | ((address & 0x100) >> 5) | ((address & 0x6000) >> 9);
  PORTC = (PORTC & 0xc0) | ((address & 0x1000) >> 12) | ((address & 0x80) >> 6) | ((address & 0x40) >> 4) | ((address & 7) << 3);

	delayMicroseconds(gConfig.pulseDelayUs);      /* Address setup */
	
	PORTD &= ~0x80;                               /* WE low */
	
	delayMicroseconds(gConfig.pulseDelayUs);      /* Address hold */
  
  PORTD = (PORTD & ~0x1c) | ((data>>1) & 0x1c);  /* Update data bits 3,4,5 */
  PORTC = (PORTC & ~0x38) | ((data<<3) & 0x38);  /* Update data bits 0,1,2 */
  
  delayMicroseconds(gConfig.pulseDelayUs);      /* Data setup */
	
	PORTD |= 0x80;                                /* WE high */
	
	delayMicroseconds(gConfig.byteDelayUs);       /* Data hold / byte write cycle time */
}


static uint16_t gEepromAddress;


void eeprom_writeImageBegin()
{
	if (gConfig.writeProtectDisable)
	{
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x80);
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x20);
		delay(gConfig.pageDelayMs);
	}

  gEepromAddress = 0;
}

bool eeprom_writeImageBlock(const uint8_t* buffer, size_t size)
{
  bool truncate = size > gConfig.size - gEepromAddress;
  if (truncate)
  {
    size = gConfig.size - gEepromAddress;
  }
  
  for (size_t i = 0; i < size; ++i)
  {
    if (gConfig.pageSize && (gEepromAddress & (gConfig.pageSize-1)) == 0)
    {
      /* Page change - wait 10ms (worst case) for write to complete */
      delay(gConfig.pageDelayMs);
      
      if (gConfig.writeProtect)
      {
        /* Locking prefix */
        writeByte(0x5555, 0xaa);
        writeByte(0x2aaa, 0x55);
        writeByte(0x5555, 0xa0);
      }
    }

    writeByte(gEepromAddress++, buffer[i]);
  }
  
  return !truncate;
}
