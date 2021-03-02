/* Basic XMODEM implementation for Raspberry Pi Pico
 *
 * This only implements the old-school checksum, not CRC, 
 * and only supports 128-byte blocks. But the whole point
 * of XMODEM was to be simple, and this works fine for ~32K
 * transfers over USB-serial.
 */

#include <Arduino.h>

#include "xmodem.h"

#include <stdio.h>
#include <string.h>

#include "serprintf.h"


const int XMODEM_SOH = 1;
const int XMODEM_EOT = 4;
const int XMODEM_ACK = 6;
const int XMODEM_DLE = 0x10;
const int XMODEM_NAK = 0x15;
const int XMODEM_CAN = 0x18;

const int XMODEM_BLOCKSIZE = 128;


xmodem_config_t xmodem_config =
{
	1,    /* logLevel */
	true, /* useCrc */
	false /* useEscape */
};


static char gLogBuffer[512];
static int gLogPos = 0;


static void xmodem_log(const __FlashStringHelper *s, ...)
{
  va_list args;
  va_start(args, s);
  int count = vsnprintf_P(gLogBuffer + gLogPos, sizeof gLogBuffer - gLogPos, (PGM_P)s, args);
  va_end(args);

  gLogPos += count;

  if (gLogPos < sizeof gLogBuffer - 1)
  {
    gLogBuffer[gLogPos++] = '\n';
    gLogBuffer[gLogPos] = 0;
  }

  if (gLogPos >= sizeof gLogBuffer - 1)
  {
    gLogPos = sizeof gLogBuffer - 1;
  }
}

void xmodem_dumplog()
{
	if (gLogPos)
	{
		Serial.println(gLogBuffer);
	}
}

static void xmodem_clearlog()
{
	gLogPos = 0;
	gLogBuffer[gLogPos] = 0;
}


void xmodem_set_config(xmodem_mode_t mode)
{
	memset(&xmodem_config, 0, sizeof xmodem_config);

	switch (mode)
	{
		case XMODEM_MODE_ORIGINAL:
			xmodem_config.useEscape = false;
			xmodem_config.useCrc = false;
			break;

		case XMODEM_MODE_CRC:
			xmodem_config.useEscape = false;
			xmodem_config.useCrc = true;
			break;
	}
}

typedef int32_t absolute_time_t;

absolute_time_t get_absolute_time() { return millis(); }
absolute_time_t make_timeout_time_ms(int32_t ms) { return millis() + ms; }

int32_t xmodem_receive(const char* message, bool (*inputhandler)(int c), bool (*datahandler)(const char*, size_t))
{
	xmodem_clearlog();

	int32_t sizeReceived = 0;
	int packetNumber = 1;

	bool eof = false;
	bool can = false;
	bool error = false;

	/* Receive a file */
	while (true)
	{
		absolute_time_t nextPrintTime = get_absolute_time();

		/* Receive next packet */
		while (true)
		{
			if (sizeReceived == 0 && get_absolute_time() > nextPrintTime)
			{
				xmodem_dumplog();

				if (message) Serial.println(message);
				
				if (xmodem_config.useCrc)
				{
          Serial.write(8);
          Serial.write('C');
				}
				else
				{
					Serial.write(XMODEM_NAK);
				}

				nextPrintTime = make_timeout_time_ms(3000);
			}

      if (Serial.available() == 0) continue;
      int c = Serial.read();

			if (c == XMODEM_EOT || c == XMODEM_SOH || c == XMODEM_CAN)
			{
				eof = (c == XMODEM_EOT);
				can = (c == XMODEM_CAN);
				break;
			}
			else if (inputhandler && inputhandler(c))
			{
				return -1;
			}
			else if (xmodem_config.logLevel >= 1)
			{
				xmodem_log(F("Unexpected %d"), c);
			}
		}

		if (eof) 
		{
			if (xmodem_config.logLevel >= 2) xmodem_log(F("EOT => ACK"));
			Serial.write(XMODEM_ACK);
			break;
		}

		if (can)
		{
			if (xmodem_config.logLevel >= 1) xmodem_log(F("CAN => ACK"));
			Serial.write(XMODEM_ACK);
			break;
		}


		if (xmodem_config.logLevel >= 2)
		{
			xmodem_log(F("SOH %d"), packetNumber);
		}


		bool timeout = false;
		absolute_time_t timeoutTime = make_timeout_time_ms(1000);

		int checksum = 0;
		bool escape = false;

		char buffer[2+XMODEM_BLOCKSIZE+2];
		int bufpos = 0;
		while (bufpos < 2+XMODEM_BLOCKSIZE + (xmodem_config.useCrc ? 2 : 1) && !timeout)
		{
			if (get_absolute_time() > timeoutTime)
			{
				if (xmodem_config.logLevel >= 1) xmodem_log(F("Timeout"));
				timeout = true;
				break;
			}

      if (!Serial.available()) continue;
			int c = Serial.read();

			if (xmodem_config.logLevel >= 3)
			{
				xmodem_log(F(" %d"), c);
			}

			bool isData = (bufpos >= 2) && (bufpos < 2+XMODEM_BLOCKSIZE);

			if (xmodem_config.useEscape && isData && c == XMODEM_DLE)
			{
				escape = true;
				continue;
			}

			if (escape) c ^= 0x40;
			escape = false;

			buffer[bufpos++] = c;

			if (isData)
			{
				if (xmodem_config.useCrc)
				{
					checksum = checksum ^ (int)c << 8;
					for (int i = 0; i < 8; ++i)
						if (checksum & 0x8000)
							checksum = checksum << 1 ^ 0x1021;
						else
							checksum = checksum << 1;
				}
				else
				{
					checksum += c;
				}
			}
		}

		bool wrongPacket = (buffer[0] != (char)packetNumber);
		bool badPacketInv = (buffer[1] != (char)(255-buffer[0]));
		bool badChecksum = (buffer[2+XMODEM_BLOCKSIZE] != (char)checksum);
		if (xmodem_config.useCrc)
		{
			badChecksum = (buffer[2+XMODEM_BLOCKSIZE] != (char)(checksum>>8))
				|| (buffer[2+XMODEM_BLOCKSIZE+1] != (char)checksum);
		}

		if (timeout || wrongPacket || badPacketInv || badChecksum)
		{
			if (xmodem_config.logLevel >= 1)
			{
			  if (timeout)
			    xmodem_log(F("NAK/Timeout"));
        else if (wrongPacket)
          xmodem_log(F("NAK/PacketNumber"));
        else if (badPacketInv)
          xmodem_log(F("NAK/PacketNumberInv"));
        else if (badChecksum)
          xmodem_log(F("NAK/Checksum"));
        else
          xmodem_log(F("NAK"));
			}
			Serial.write(XMODEM_NAK);
			continue;
		}

    sizeReceived += XMODEM_BLOCKSIZE;
    packetNumber++;

    if (!datahandler(buffer+2, XMODEM_BLOCKSIZE)) 
    {
      if (xmodem_config.logLevel >= 1) xmodem_log(F("Error: truncated"));
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      Serial.write(XMODEM_CAN);
      break;
    }

	  if (xmodem_config.logLevel >= 2) xmodem_log(F("ACK"));
    Serial.write(XMODEM_ACK);
  }

	Serial.println("");
	xmodem_dumplog();
	xmodem_clearlog();

	if (can || error) return -1;

	return sizeReceived;
}
