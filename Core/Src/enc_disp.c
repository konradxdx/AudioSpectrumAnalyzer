/*
 * disp.c
 *
 *  Created on: Dec 10, 2022
 *      Author: Konrad
 */
#include <enc_disp.h>
#include "usart.h"
#include "tim.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

void dprint(const char *fmt, ...) {
  char buffer[40] = {0};
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  for(int i = 0 ; i<40 ; i++){
	 if(buffer[i]<32) buffer[i]=' ';
  }
  HAL_UART_Transmit(&huart1, (uint8_t*)buffer, 40, -1);

}

void clearDisp(void) {
	const uint8_t x =  19 ;
	HAL_UART_Transmit(&huart1, &x, 1, -1);
}

void UpdateEnc(uint8_t min, uint8_t max, int16_t* enc_state, bool stopAtBorder) {
	static uint16_t LastTimerCounter = 0;
	int TimerDif = htim2.Instance->CNT - LastTimerCounter;
	if (TimerDif >= 4 || TimerDif <= -4) {
		TimerDif /= 4;
		*enc_state += (int8_t) TimerDif;
		if (*enc_state > max){
			if(stopAtBorder == true) *enc_state = max; else *enc_state = min;
		}
		if (*enc_state < min){
			if(stopAtBorder == true) *enc_state = min; else *enc_state = max;
		}
		LastTimerCounter = htim2.Instance->CNT;
	}
}

bool button(void){
	if(HAL_GPIO_ReadPin(SW_GPIO_Port, SW_Pin)==1) return false; else return true;
}
