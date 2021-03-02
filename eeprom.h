#ifndef INCLUDED_EEPROM_H
#define INCLUDED_EEPROM_H

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void eeprom_init();

void eeprom_writeImageBegin();
bool eeprom_writeImageBlock(const uint8_t* buffer, size_t size);

#endif
