#include "serprintf.h"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

static char buffer[256];

int serprintf(const __FlashStringHelper* format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf_P(buffer, sizeof buffer, (PGM_P)format, args);
  Serial.print(buffer);
  va_end(args);
}
