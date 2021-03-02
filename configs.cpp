#include "configs.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodem.h"
#include "serprintf.h"


picoprom_config_t gConfig;


const static picoprom_config_t gConfigs[] =
{
	{
		"AT28C256",
		32768,
		64, 10, 1, 0,
		true, false
	},
	{
		"AT28C256F",
		32768,
		64, 3, 1, 0,
		true, false
	},
	{
		"AT28C64",
		8192,
		0, 10, 1, 1000,
		false, false
	},
	{
		"AT28C64B",
		8192,
		64, 10, 1, 0,
		true, false
	},
	{
		"AT28C64E",
		8192,
		0, 10, 1, 200,
		false, false
	},
	{
		"AT28C16",
		2048,
		0, 10, 1, 1000,
		false, false
	},
	{
		"AT28C16E",
		2048,
		0, 10, 1, 200,
		false, false
	},
	{
		"M28C16",
		2048,
		64, 3, 1, 0,
		true, false
	},
	{
		NULL
	}
};


static int gConfigIndex = 0;


void init_settings()
{
	gConfigIndex = 0;
	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	xmodem_config.logLevel = 1;
}


void show_settings()
{
	printf(F("EEPROM Device: %s\n"), gConfig.name);
	printf(F("        Capacity: %dK bytes\n"), gConfig.size / 1024);
	printf(F("        Page mode: %s\n"), gConfig.pageSize ? "on" : "off");
	if (gConfig.pageSize)
	{
		printf(F("        Page size: %d bytes\n"), gConfig.pageSize);
		printf(F("        Page delay: %dms\n"), gConfig.pageDelayMs);
	}
	printf(F("        Pulse delay: %dus\n"), gConfig.pulseDelayUs);
	printf(F("        Byte delay: %dus\n"), gConfig.byteDelayUs);
	printf(F("        Write protect: %s\n"), gConfig.writeProtect ? "enable" : gConfig.writeProtectDisable ? "disable" : "no action / not supported");
	printf(F("\n"));
	printf(F("Serial protocol: XMODEM+CRC\n"));
	printf(F("        Block size: 128 bytes\n"));
	printf(F("        CRC: on\n"));
	printf(F("        Escaping: off\n"));
	printf(F("        Log level: %d\n"), xmodem_config.logLevel);
}


static void change_device()
{
	++gConfigIndex;
	if (!gConfigs[gConfigIndex].name)
		gConfigIndex = 0;

	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	printf(F("\n\nChanged device to %s\n"), gConfig.name);
}

static void change_log_level()
{
	xmodem_config.logLevel = (xmodem_config.logLevel + 1) % 4;

	printf(F("\n\nChanged log level to %d\n"), xmodem_config.logLevel);
}


typedef struct
{
	char key;
	const char* commandName;
	void (*action)();
} command_t;

static command_t gCommands[] =
{
	{ 'd', "change device", change_device },
	{ 'l', "change log level", change_log_level },
	{ 'p', "return to programming mode", NULL },
	{ 0 }
};


void change_settings()
{
	while (true)
	{
		printf(F("\n\n"));
		printf(F("\n\n"));
		printf(F("\n\n"));

		show_settings();
		
		printf(F("\n\n"));
		
		printf(F("Changing settings:\n"));
		printf(F("\n"));
		
		for (int i = 0; gCommands[i].key; ++i)
		{
			printf(F("    %c = %s\n"), gCommands[i].key, gCommands[i].commandName);
		}
		printf(F("\n"));
		printf(F("?"));

    while (!Serial.available());
		int c = Serial.read();
		for (int i = 0; gCommands[i].key; ++i)
		{
			if (c == gCommands[i].key)
			{
				if (!gCommands[i].action)
					return;

				gCommands[i].action();
			}
		}
	}
}
