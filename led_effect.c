/*
 * led_effect.c
 *
 *  Created on: 13-Sept-2026
 *      Author: Kalaivani
 */
#include "main.h"

void turn_on_all_leds(void){
	HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin, GPIO_PIN_SET);
}

void turn_off_all_leds(void){
	HAL_GPIO_WritePin(LD2_GPIO_Port,LD2_Pin, GPIO_PIN_RESET);
}

void led_effect_stop(void){
	for(int i=0; i<2; i++)
		xTimerStop(handle_led_timer[i], portMAX_DELAY);
}

void led_effect(int n){
	led_effect_stop();
	xTimerStart(handle_led_timer[n-1], portMAX_DELAY);
}


void led_effect1(void){
	static int flag = 1;
	(flag ^= 1) ? turn_off_all_leds() : turn_on_all_leds();
}

void led_effect2(void){
	static int flag = 1;
	(flag ^= 1) ? turn_off_all_leds() : turn_on_all_leds();
}

