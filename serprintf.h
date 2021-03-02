#ifndef INCLUDED_SERPRINTF_H
#define INCLUDED_SERPRINTF_H

#pragma once

#include <Arduino.h>

int serprintf(const __FlashStringHelper* format, ...);
#define printf serprintf

#endif
