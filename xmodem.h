#ifndef INCLUDED_XMODEM_H
#define INCLUDED_XMODEM_H

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* Protocol variants */
typedef enum
{
	XMODEM_MODE_ORIGINAL,
	XMODEM_MODE_CRC
} xmodem_mode_t;


/* Configuration parameters */
typedef struct
{
	/* Enable/disable logging during the transfer.
	 *
	 * 0 = fatal errors only
	 * 1 = errors only
	 * 2 = verbose logging
	 * 3 = log all bytes received
	 */
	int logLevel;

	/* Enable the CRC variant of the protocol */
	bool useCrc;

	/* Enable decoding of escape characters */
	bool useEscape;
} xmodem_config_t;


/* Currently active configuration */
extern xmodem_config_t xmodem_config;


/* Change configuration */
void xmodem_set_config(xmodem_mode_t mode);


/* Receive a file.  Note that XMODEM pads files to the next multiple of 128, 
 * using character 26.
 *
 * Returns the amount of data received - always a multiple of 128.
 *
 *    message:      an optional prompt to start the transfer
 *    inputhandler: an optional function to handle unexpected input before the transfer
 *    datahandler:  a function that's called for each 128-byte block of data received
 *
 * If message is provided, it will be printed every few seconds until the 
 * first data is received.
 *
 * If inputhandler is provided, it will be called if unexpected input is received before
 * the transfer begins.  The unexpected input character will be passed to the handler,
 * which should return 'true' if it handled it.
 * 
 * datahandler will be called repeatedly as data is received.  If it returns false, the 
 * transfer will be aborted.
 */
int32_t xmodem_receive(const char *message, bool (*inputhandler)(int), bool (*datahandler)(const char*, size_t));


/* Dumps cached log data to stdout. */
void xmodem_dumplog();


#endif
