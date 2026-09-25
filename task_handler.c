#include "main.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"
const char *msg_rtc_ampm = "Enter AM/PM (A/P): ";
typedef enum
{
    HH_CONFIG,
    MM_CONFIG,
    SS_CONFIG,
	AMPM_CONFIG,
    DATE_CONFIG,
    MONTH_CONFIG,
    DAY_CONFIG,
    YEAR_CONFIG
} rtc_config_state_t;
Student student[]=
				{
						{101, "Ravi"},
						{102, "Priya"},
						{103, "Arun"},
						{104, "Meena"},
						{105, "Kiran"},

				};
RTC_TimeTypeDef time;
RTC_DateTypeDef date;
TimerHandle_t rtc_timer;
TaskHandle_t handle_attendance_task;
rtc_config_state_t rtc_state;
uint8_t getnumber(uint8_t *p, int len);
void process_command(command_t *cmd);
int extract_command(command_t *cmd);
void attendance_task(void *argument);
void menu_task(void *param){
	uint32_t cmd_addr;
	command_t *cmd;
	const char* msg_menu = "------menu-----\n"
			     "LED effect-->0\n"
			     "Date and time_->1\n"
			     "Attendance----->2\n"
			     "Exit----->3\n"
			     "Enter your choice here: ";
	const char *msg_inv = "Invalid command\r\n";
	uint32_t option;
	while(1){
		xQueueSend(q_print, &msg_menu, portMAX_DELAY);
        xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
        cmd = (command_t *)cmd_addr;
        if(cmd -> len == 1){
        	option = cmd->payload[0] - 48;
        	switch(option)
        	{
        	case 0:
        		curr_state = sLedEffect;
        		xTaskNotify(handle_led_menu_task, 0, eNoAction);
        		break;
        	case 1:
        	     curr_state = sRtcMenu;
        	     xTaskNotify(handle_rtc_task, 0, eNoAction);
        	     break;
        	case 2:
        	    curr_state = sAttendance;

        	    const char *msg = "\r\nATTENDANCE CASE REACHED\r\n";
        	    xQueueSend(q_print, &msg, portMAX_DELAY);

        	    xTaskNotify(handle_attendance_task, 0, eNoAction);
        	    break;
        	case 3:
        	     curr_state = sLedEffect;
        	     break;
        	default:
        		xQueueSend(q_print, &msg_menu, portMAX_DELAY);
        		continue;
        	}
        }
        else{
        	xQueueSend(q_print, (uint32_t*)&msg_inv, portMAX_DELAY);
        }
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}
void cmd_task(void *param)
{
    BaseType_t ret;
    uint8_t cmd_addr;
    command_t cmd;

    while(1)
    {
        ret = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        if(ret == pdTRUE)
        {
            if(extract_command(&cmd) == 0)
            {
                HAL_UART_Transmit(&huart2,
                                  (uint8_t*)"CMD_TASK GOT: ",
                                  14,
                                  HAL_MAX_DELAY);

                HAL_UART_Transmit(&huart2,
                                  (uint8_t*)cmd.payload,
                                  strlen((char*)cmd.payload),
                                  HAL_MAX_DELAY);

                HAL_UART_Transmit(&huart2,
                                  (uint8_t*)"\r\n",
                                  2,
                                  HAL_MAX_DELAY);

                process_command(&cmd);
            }
        }
    }
}

void process_command(command_t *cmd){
	char debug_msg[50];

	sprintf(debug_msg, "CMD=%s STATE=%d\r\n",cmd->payload, curr_state);
	HAL_UART_Transmit(&huart2, (uint8_t*)debug_msg,strlen(debug_msg), HAL_MAX_DELAY);
	switch(curr_state){
	case sMainMenu:
		xTaskNotify(handle_menu_task, (uint32_t)cmd, eSetValueWithOverwrite);
	break;
	case sLedEffect:
		xTaskNotify(handle_led_menu_task, (uint32_t)cmd, eSetValueWithOverwrite);
	break;
	//case sRtcMenu:
	case sAttendance:
	{
	    uint16_t stud_id = getnumber(cmd->payload, cmd->len);

	    BaseType_t ret = xQueueSend(q_attendance, &stud_id, portMAX_DELAY);

	    char debug_msg[50];
	    sprintf(debug_msg, "ID=%d QUEUE=%d\r\n", stud_id, ret);

	    HAL_UART_Transmit(&huart2, (uint8_t *)debug_msg,
	                      strlen(debug_msg), HAL_MAX_DELAY);

	    break;
	}
	case sRtcTimeConfig:
	case sRtcDateConfig:
	case sRtcReport:
		xTaskNotify(handle_rtc_task, (uint32_t)cmd, eSetValueWithOverwrite);
	break;

	}
}

int extract_command(command_t *cmd){
	uint8_t item;
	BaseType_t status;

	//status = uxQueueMessagesWaiting(q_data);
	//if(!status)
	//	return -1;
	uint8_t i = 0;

	do{
		status = xQueueReceive(q_data, &item, portMAX_DELAY);
		if(status == pdTRUE){
			 if(item != '\r')
			cmd->payload[i++] = item;
		}
	}while(item != '\n');

	cmd->payload[i-1] = '\0';
	cmd->len = i-1;
	return 0;
}

void print_task(void *param){
	uint32_t msg;
	while(1){
		xQueueReceive(q_print, &msg, portMAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen((char*)msg), HAL_MAX_DELAY);
	}
}
uint8_t getnumber(uint8_t *p, int len);
void rtc_task(void *param){
	const char* msg_rtc1 = "----RTC----\n"
	                       "Configure_time------>0\n"
			               "Configure_date------>1\n"
			               "Configure_reporting------>2\n"
			               "Exit------>3\n"
			               "Enter your choice here :";
	const char *msg_rtc_hh = "Enter hour(1-12)";
	const char *msg_rtc_mm = "Enter minutes(0-59)";
	const char *msg_rtc_ss = "Enter seconds(0-59)";

	const char *msg_rtc_dd = "Enter date(1-31)";
	const char *msg_rtc_m = "Enter month(1-12)";
	const char *msg_rtc_day = "Enter day(1-7 , sunday:1)";
	const char *msg_rtc_year = "Enter year(00-99)";
	const char *msg_conf = "Configuration successful";
	const char *msg_rtc_report = "Enable time/date reporting(y/n)";

	uint32_t cmd_addr;
	uint32_t menu_code;
	uint32_t msg_inv;
	command_t *cmd;

	while(1){
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		xQueueSend(q_print, &msg_rtc1, portMAX_DELAY);
		while(curr_state != sMainMenu){
			xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
			cmd = (command_t*)cmd_addr;
			switch(curr_state){
			case sRtcMenu:{
				if(cmd->len == 1)
				{
					menu_code = cmd-> payload[0] - 48;
					switch(menu_code)
					{
					case 0:
						curr_state = sRtcTimeConfig;
						rtc_state = HH_CONFIG;
						xQueueSend(q_print, &msg_rtc_hh, portMAX_DELAY);
						break;
					case 1:
						curr_state = sRtcDateConfig;
						rtc_state = DATE_CONFIG;
						xQueueSend(q_print, &msg_rtc_dd, portMAX_DELAY);
						break;
					case 2:
						curr_state = sRtcReport;
						xQueueSend(q_print, &msg_rtc_report, portMAX_DELAY);
					    break;
					default:
						curr_state = sMainMenu;
						xQueueSend(q_print, &msg_inv, portMAX_DELAY);
					}
				}
				break;
			}
			case sRtcTimeConfig:
			{
				switch(rtc_state)
				{
				case HH_CONFIG:{
					uint8_t hour = getnumber(cmd->payload, cmd->len);
					time.Hours = hour;
					rtc_state = MM_CONFIG;
					xQueueSend(q_print, &msg_rtc_mm, portMAX_DELAY);
					break;
				}
				case MM_CONFIG:
				{
					uint8_t min = getnumber(cmd->payload, cmd->len);
					time.Minutes = min;
					rtc_state = SS_CONFIG;
					xQueueSend(q_print, &msg_rtc_ss, portMAX_DELAY);
					break;
				}
				case SS_CONFIG:
				{
					uint8_t sec=  getnumber(cmd->payload, cmd->len);
					time.Seconds = sec;

				if(!validate_rtc_information(&time, NULL))
				{
					rtc_state = AMPM_CONFIG;
					xQueueSend(q_print, &msg_rtc_ampm, portMAX_DELAY);

				}
				else
					{xQueueSend(q_print, &msg_inv, portMAX_DELAY);
					curr_state = sMainMenu;
					rtc_state = HH_CONFIG;
					break;
				}

				break;
			}
			case AMPM_CONFIG:
			{
				if(cmd->len == 1)
				{
					if(cmd->payload[0] == 'A' || cmd->payload[0] == 'a')
					{
						time.TimeFormat = RTC_HOURFORMAT12_AM;
					}
					else if(cmd->payload[0] == 'P' || cmd->payload[0] == 'p')
					{
						time.TimeFormat = RTC_HOURFORMAT12_PM;
					}
			        else
			        {
			            xQueueSend(q_print, &msg_inv, portMAX_DELAY);
			            break;
			        }
			        rtc_configure_time(&time);

			        xQueueSend(q_print, &msg_conf, portMAX_DELAY);

			        show_time_date();

			        curr_state = sMainMenu;
			        rtc_state = HH_CONFIG;
				}
				break;
			}

			}
				break;
		}
			case sRtcDateConfig:
			{
				switch(rtc_state)
				{
					case DATE_CONFIG:{
					    uint8_t d = getnumber(cmd->payload , cmd->len);
					    date.Date = d;
					    if(!validate_rtc_information(NULL, &date))
					    {
					        rtc_state = MONTH_CONFIG;
					        xQueueSend(q_print, &msg_rtc_m, portMAX_DELAY);   // ✅ month
					    }
					    else
					    {
					        xQueueSend(q_print, &msg_inv, portMAX_DELAY);
					    }
						break;}
					case MONTH_CONFIG:{
						uint8_t month = getnumber(cmd->payload , cmd->len);
						date.Month = month;
						rtc_state = DAY_CONFIG;
						xQueueSend(q_print,&msg_rtc_day,portMAX_DELAY);
						break;}
					case DAY_CONFIG:{
						uint8_t day = getnumber(cmd->payload , cmd->len);
						date.WeekDay = day;
						rtc_state = YEAR_CONFIG;
						xQueueSend(q_print,&msg_rtc_year,portMAX_DELAY);
						break;
					}
					case YEAR_CONFIG:{
						uint8_t year = getnumber(cmd->payload , cmd->len);
						date.Year = year;

						if(!validate_rtc_information(NULL,&date))
						{
							rtc_configure_date(&date);
							xQueueSend(q_print,&msg_conf,portMAX_DELAY);
							show_time_date();

						}else
							{xQueueSend(q_print,&msg_inv,portMAX_DELAY);

							curr_state = sMainMenu;
							rtc_state = DATE_CONFIG;
							break;}
						}

				}
			break;
			}
			case sRtcReport:
			{
				if(cmd->len == 1)
				{
					if(cmd->payload[0] == 'y'){
						if(xTimerIsTimerActive(rtc_timer) == pdFALSE)
							xTimerStart(rtc_timer,portMAX_DELAY);
					}else if (cmd->payload[0] == 'n'){
						xTimerStop(rtc_timer,portMAX_DELAY);
					}else{
						xQueueSend(q_print,&msg_inv,portMAX_DELAY);
					}

				}else
				    xQueueSend(q_print,&msg_inv,portMAX_DELAY);

				curr_state = sMainMenu;


				break;
			}

			}

		}
	}
	xTaskNotify(handle_menu_task, 0, eNoAction);
}

uint8_t getnumber(uint8_t *p , int len)
{

	int value ;
	//uint16_t stud_id;
	//const char cmd;
	if(len == 3)
	    value = ((p[0]-48) * 100) + ((p[1]-48) * 10) + (p[2]-48);
	else if(len == 2)
	    value = ((p[0]-48) * 10) + (p[1]-48);
	else
	    value = p[0] - 48;

	return value;

}


void led_task(void *param){
	uint32_t cmd_addr;
	command_t *cmd;
	const char *msg_inv = "Invalid command\r\n";
	const char* msg_led = "-----led_effect----\n"
			              "(none, e1)\n"
			              "Enter your choice here : ";
	while(1){
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		xQueueSend(q_print, &msg_led, portMAX_DELAY);
		xTaskNotifyWait(0, 0, &cmd_addr, portMAX_DELAY);
		cmd = (command_t*)cmd_addr;
		if(cmd->len<=4){
			if(!strcmp((char*)cmd->payload,"none"))
				led_effect_stop();
			else if(!strcmp((char*)cmd->payload,"e1"))
				led_effect(1);
			else if(!strcmp((char*)cmd->payload,"e2"))
				led_effect(2);
			else
				{
				xQueueSend(q_print, &msg_inv, portMAX_DELAY);
			    continue;
				}
		}
		else{
			xQueueSend(q_print, (uint32_t*)&msg_inv, portMAX_DELAY);
		}
		curr_state = sMainMenu;
		xTaskNotify(handle_menu_task, 0, eSetValueWithOverwrite);
	}
}

void attendance_task(void *argument){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		const char *msg = "\r\n----Attendance System----\r\n"
				          "Enter Student ID";
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		uint16_t stud_id;
		bool found = false;
		int student_index = -1;
		BaseType_t ret;

		ret = xQueueReceive(q_attendance, &stud_id, portMAX_DELAY);

		if(ret == pdTRUE)
		{
		    const char *msg = "QUEUE RX OK\r\n";
		    xQueueSend(q_print, &msg, portMAX_DELAY);
		}
		else
		{
		    const char *msg = "QUEUE RX FAILED\r\n";
		    xQueueSend(q_print, &msg, portMAX_DELAY);
		}
		const char *debug2 = "AFTER QUEUE RECEIVE\r\n";
		xQueueSend(q_print, &debug2, portMAX_DELAY);
		char id_debug[30];
		sprintf(id_debug, "STUD_ID=%d\r\n", stud_id);
		HAL_UART_Transmit(&huart2, (uint8_t*)id_debug,
		                  strlen(id_debug), HAL_MAX_DELAY);
		for(int i = 0; i < NUM_STUDENTS; i++)
		{
		    char debug[50];

		    sprintf(debug, "CHECK i=%d ID=%d\r\n",
		            i, student[i].id);

		    HAL_UART_Transmit(&huart2,
		                      (uint8_t *)debug,
		                      strlen(debug),
		                      HAL_MAX_DELAY);

		    if(stud_id == student[0].id)
		    {
		        const char *msg = "MATCHED RAVI\r\n";
		        xQueueSend(q_print, &msg, portMAX_DELAY);
		    }
		    else
		    {
		        const char *msg = "NOT MATCHED\r\n";
		        xQueueSend(q_print, &msg, portMAX_DELAY);
		    }
	}
}
}




