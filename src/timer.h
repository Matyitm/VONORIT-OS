#ifndef VONORIT_TIMER_H
#define VONORIT_TIMER_H

#include <stdint.h>

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} timer_datetime_t;

void timer_initialize(void);
void timer_tick_handler(void);
uint64_t timer_get_ticks(void);
uint64_t timer_get_seconds(void);
timer_datetime_t timer_get_datetime(void);
uint64_t timer_get_wall_seconds(void);
uint64_t timer_get_unix_days(uint16_t year, uint8_t month, uint8_t day);


#endif
