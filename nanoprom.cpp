#include <Arduino.h>
#include <stdio.h>

#include "configs.h"
#include "xmodem.h"
#include "eeprom.h"
#include "serprintf.h"


void setup()
{
  /* Initialize the eeprom module first, so we don't accidentally write garbage to it */
  eeprom_init();

  Serial.begin(115200);
  while (!Serial.available()) {
    printf(F("Press a key to begin\n"));
    delay(1000);
  }
  while (Serial.available()) Serial.read();

  init_settings();
}


static void banner()
{
  printf(F("\n\n"));
  printf(F("\n\n"));
  printf(F("\n\n"));

  printf(F("NanoPROM v0.1    Arduino Nano DIP-EEPROM programmer\n"));
  printf(F("                 by George Foot, March 2021\n"));
  printf(F("                 https://github.com/gfoot/nanoprom\n"));
}



static bool input_handler(int c)
{
  if (c == 13)
  {
    change_settings();
    return true;
  }
  return false;
}


static bool gWriteStarted = false;

static bool data_handler(const uint8_t* buffer, size_t size)
{
  if (!gWriteStarted)
  {
    gWriteStarted = true;
    eeprom_writeImageBegin();
  }
  return eeprom_writeImageBlock(buffer, size);
}


void loop()
{
  serprintf(F("\n\n"));
  serprintf(F("\n\n"));
  serprintf(F("\n\n"));

  banner();

  serprintf(F("\n\n"));
  
  show_settings();
  
  serprintf(F("\n\n"));
  printf(F("Ready to program - send data now, or press Enter to change settings\n"));
  printf(F("\n"));

  gWriteStarted = false;
  
  int32_t sizeReceived = xmodem_receive(NULL, input_handler, data_handler);

  if (sizeReceived < 0)
  {
    xmodem_dumplog();
    printf(F("XMODEM transfer failed\n"));
  }

  if (sizeReceived <= 0)
  {
    return;
  }

  printf(F("\n"));
  printf(F("Transfer complete - %ld bytes written to EEPROM\n"), sizeReceived);
  printf(F("\n"));
}
