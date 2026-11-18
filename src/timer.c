#include <stdint.h>

#include "timer.h"

#define PIT_CHANNEL0 0x40u
#define PIT_COMMAND 0x43u
#define PIT_FREQUENCY 1193182u
#define TIMER_HZ 100u

static volatile uint64_t timer_ticks;
static timer_datetime_t rtc_boot;
static uint64_t rtc_base_seconds;

static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));
	return value;
}

static uint8_t rtc_read(uint8_t register_index)
{
	outb(0x70u,register_index);
	return inb(0x71u);
}

static uint8_t bcd_to_binary(uint8_t value)
{
	return (uint8_t)((value&0x0Fu)+((value>>4)*10u));
}

static timer_datetime_t rtc_read_datetime(void)
{
	timer_datetime_t current;
	uint8_t register_b;
	for(;;)
	{
		if((rtc_read(0x0Au)&0x80u)==0u) break;
	}
	uint8_t seconds=rtc_read(0x00u);
	uint8_t minutes=rtc_read(0x02u);
	uint8_t hours=rtc_read(0x04u);
	uint8_t day=rtc_read(0x07u);
	uint8_t month=rtc_read(0x08u);
	uint8_t year=rtc_read(0x09u);
	register_b=rtc_read(0x0Bu);

	if((register_b&0x04u)==0u)
	{
		seconds=bcd_to_binary(seconds);
		minutes=bcd_to_binary(minutes);
		hours=bcd_to_binary(hours);
		day=bcd_to_binary(day);
		month=bcd_to_binary(month);
		year=bcd_to_binary(year);
	}

	uint8_t pm=(uint8_t)(hours&0x80u);
	if((register_b&0x02u)==0u)
	{
		hours=(uint8_t)(hours&0x7Fu);
		if(hours==12u) hours=pm?12u:0u;
		else if(pm!=0u) hours=(uint8_t)(hours+12u);
	}

	current.year=(uint16_t)(year<100u?2000u+year:year);
	current.month=month;
	current.day=day;
	current.hour=hours;
	current.minute=minutes;
	current.second=seconds;
	return current;
}

uint64_t timer_get_unix_days(uint16_t year, uint8_t month, uint8_t day)
{
	int64_t y=(int64_t)year;
	int64_t m=(int64_t)month;
	int64_t d=(int64_t)day;
	if(m<=2){y-=1;m+=12;}
	int64_t era=(y>=0?y:y-399)/400;
	int64_t yoe=y-era*400;
	int64_t mp=m+(m>2?-3:9);
	int64_t doe=yoe*365+yoe/4-yoe/100+(306*mp+5)/10+(d-1);
	int64_t days=era*146097+doe-719468;
	return (uint64_t)days;
}

static uint64_t datetime_seconds(timer_datetime_t value)
{
	uint64_t days=timer_get_unix_days(value.year,value.month,value.day);
	return days*86400u+(uint64_t)value.hour*3600u+(uint64_t)value.minute*60u+(uint64_t)value.second;
}

void timer_initialize(void)
{
	timer_ticks=0u;
	rtc_boot=rtc_read_datetime();
	rtc_base_seconds=datetime_seconds(rtc_boot);
	uint16_t divisor=(uint16_t)(PIT_FREQUENCY/TIMER_HZ);
	outb(PIT_COMMAND,0x36u);
	outb(PIT_CHANNEL0,(uint8_t)(divisor&0xFFu));
	outb(PIT_CHANNEL0,(uint8_t)(divisor>>8));
}

void timer_tick_handler(void)
{
	++timer_ticks;
}

uint64_t timer_get_ticks(void)
{
	return timer_ticks;
}

uint64_t timer_get_seconds(void)
{
	return timer_ticks/TIMER_HZ;
}

uint64_t timer_get_wall_seconds(void)
{
	return rtc_base_seconds+timer_get_seconds();
}

timer_datetime_t timer_get_datetime(void)
{
	timer_datetime_t result=rtc_boot;
	uint64_t wall_seconds=timer_get_wall_seconds();
	uint64_t day_seconds=wall_seconds%86400u;
	result.hour=(uint8_t)(day_seconds/3600u);
	result.minute=(uint8_t)((day_seconds/60u)%60u);
	result.second=(uint8_t)(day_seconds%60u);

	uint64_t days=wall_seconds/86400u;
	for(int64_t year=1970;year<2100;++year)
	{
		uint64_t next=timer_get_unix_days((uint16_t)(year+1),1u,1u);
		if(next>days)
		{
			result.year=(uint16_t)year;
			uint64_t start=timer_get_unix_days((uint16_t)year,1u,1u);
			uint64_t month_day=days-start;
			for(uint8_t month=1u;month<=12u;++month)
			{
				uint8_t next_month=(uint8_t)(month==12u?1u:month+1u);
				uint16_t next_year=(uint16_t)(month==12u?year+1:year);
				uint64_t month_days=timer_get_unix_days(next_year,next_month,1u)-timer_get_unix_days((uint16_t)year,month,1u);
				if(month_day<month_days)
				{
					result.month=month;
					result.day=(uint8_t)(month_day+1u);
					break;
				}
				month_day-=month_days;
			}
			break;
		}
	}
	return result;
}
