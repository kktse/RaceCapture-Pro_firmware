/*
 * serial.h

 *
 *  Created on: Jan 10, 2013
 *      Author: brent
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include "FreeRTOS.h"

typedef struct _Serial{

	char (*get_c_wait)(portTickType delay);
	char (*get_c)(void);

	int (*get_line)(char *s, int len);
	int (*get_line_wait)(char *s, int len, portTickType delay);

	int (*put_c)(char c);
	int (*put_s)(const char *);

	void (*flush)(void);

} Serial;


void init_serial(void);

Serial * get_serial_usart0();

Serial * get_serial_usart1();

Serial * get_serial_usb();


void put_int(Serial * serial, int n);

void put_hex(Serial * serial, int n);

void put_float(Serial * serial, float f, int precision);

void put_double(Serial * serial, double f, int precision);

void put_uint(Serial * serial, unsigned int n);

void put_nameUint(Serial * serial, const char *s, unsigned int n);

void put_nameSuffixUint(Serial * serial, const char *s, const char *suf, unsigned int n);

void put_nameIndexUint(Serial * serial, const char *s, int i, unsigned int n);

void put_nameInt(Serial * serial, const char *s, int n);

void put_nameSuffixInt(Serial * serial, const char *s, const char *suf, int n);

void put_nameIndexInt(Serial * serial, const char *s, int i, int n);

void put_nameDouble(Serial * serial, const char *s, double n, int precision);

void put_nameSuffixDouble(Serial * serial, const char *s, const char *suf, double n, int precision);

void put_nameIndexDouble(Serial * serial, const char *s, int i, double n, int precision);

void put_nameFloat(Serial * serial, const char *s, float n, int precision);

void put_nameSuffixFloat(Serial * serial, const char *s, const char *suf, float n, int precision);

void put_nameIndexFloat(Serial * serial, const char *s, int i, float n, int precision);

void put_nameString(Serial * serial, const char *s, const char *v);

void put_nameSuffixString(Serial * serial, const char *s, const char *suf, const char *v);

void put_nameIndexString(Serial * serial, const char *s, int i, const char *v);

void put_nameEscapedString(Serial * serial, const char *s, const char *v, int length);

void put_bytes(Serial *serial, char *data, unsigned int length);

void put_crlf(Serial * serial);



#endif /* SERIAL_H_ */
