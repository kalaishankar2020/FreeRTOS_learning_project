/*
 * rtc.c
 *
 *  Created on: 14-Sept-2026
 *      Author: Kalaivani
 */
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdio.h>
void show_time_date(void)
{
	static char showtime[40];
	static char showdate[40];

	RTC_DateTypeDef rtc_date;
	RTC_TimeTypeDef rtc_time;

	static char *time = showtime;
	static char *date = showdate;

	HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

	char *format;
    HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
	format = (rtc_time.TimeFormat == RTC_HOURFORMAT12_AM)? "AM" : "PM";

	//Display Time Format
	sprintf(showtime, "\r\nCurrent Time: %02d:%02d:%02d %s\r\n", rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds, format);
	xQueueSend(q_print, &time, portMAX_DELAY);

	//Display Date Format
	sprintf((char*)showdate, "\tDate: %02d-%02d-%2d\n", rtc_date.Month, rtc_date.Date, 2000+rtc_date.Year);
	xQueueSend(q_print, &date, portMAX_DELAY);
}


void rtc_configure_time(RTC_TimeTypeDef *time)
{

	HAL_RTC_SetTime(&hrtc, time, RTC_FORMAT_BIN);
}

void rtc_configure_date(RTC_DateTypeDef *date)
{
	HAL_RTC_SetDate(&hrtc, date, RTC_FORMAT_BIN);
}

int validate_rtc_information(RTC_TimeTypeDef *time , RTC_DateTypeDef *date)
{
	if(time){
		if( (time->Hours > 12) || (time->Minutes > 59) || (time->Seconds > 59) )
			return 1;
	}

	if(date){
		if( (date->Date > 31) || (date->WeekDay > 7) || (date->Year > 99) || (date->Month > 12) )
			return 1;
	}

	return 0;
}
