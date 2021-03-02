#ifndef INCLUDED_CONFIGS_H
#define INCLUDED_CONFIGS_H

#pragma once

#include <stdbool.h>
#include <stdint.h>


typedef struct
{
	const char *name;
	uint32_t size;
	int pageSize;
	int pageDelayMs;
	int pulseDelayUs;
	int byteDelayUs;
	bool writeProtect;
	bool writeProtectDisable;
} picoprom_config_t;


extern picoprom_config_t gConfig;


void init_settings();
void show_settings();
void change_settings();


#endif
