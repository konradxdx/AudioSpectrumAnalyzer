/*
 * clean_main.c
 *
 *  Created on: 15 gru 2022
 *      Author: Konrad
 */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdbool.h>

#include "enc_disp.h"
//#include "dac_driver.h"

int16_t stan_enc = 20;
uint8_t znak = 127;
char pasek[20];
volatile uint16_t samples[2];

typedef struct menu_option{
	char * prompt;
	void (*func)(void);
}menu_option;


void line(){
	HAL_Delay(500);
	while (button()==0) {
		UpdateEnc(0, 20, &stan_enc, true);
		for (int i = 0; i < 20; i++) {
			if (i < stan_enc) {
				pasek[i] = znak;
			}
			if (i < 20 && i >= stan_enc) {
				pasek[i] = 'D';
			}
			if (i >= 20)
				pasek[i] = ' ';
		}

		dprint("%.20s^Status",pasek);
		HAL_Delay(10);
	}
	HAL_Delay(200);
	clearDisp();
	stan_enc = znak;
}

void change_char(){
	HAL_Delay(500);
	while (button()==0) {
		znak = stan_enc;
		UpdateEnc(32, 127, &stan_enc, false);
		dprint("Kod: %03d = '%c'", stan_enc, stan_enc);
		HAL_Delay(10);
	}
	clearDisp();
	HAL_Delay(200);
}
/*
void set_dac(){
	int16_t dac_channel=0;
	int16_t dac_value=0;
	HAL_Delay(100);
	while(button()==0){
		UpdateEnc(0,3,&dac_channel,0);
		dprint("Kanal ADC: \"%d\"",dac_channel);
		HAL_Delay(100);
	}
	HAL_Delay(100);
	while(button()==0){
		UpdateEnc(0,127,&dac_value,1);
		dprint("Wartosc ADC: \"%d\"",dac_value);
		dac_write(dac_channel, dac_value, 0);
		HAL_Delay(100);
	}
}
*/
void menu(){

	struct menu_option options[] = {
			{"Display bar",line},
			{"Set bar character",change_char},
			//{"Drive DAC", set_dac}
	};

	int16_t position = 0;
	uint8_t options_count = sizeof(options)/sizeof(options[0]);

	while(button()==0){
		UpdateEnc(0,options_count-1, &position, false);
		dprint("%d.%s",position+1, options[position].prompt);
		HAL_Delay(50);
	}
	options[position].func();
}


void clean_main(){
	HAL_TIM_Base_Start(&htim2);
	//HAL_TIM_Base_Start(&htim3);
	HAL_ADC_Start_DMA(&hadc1, samples, 2);
}

void clean_loop(){
	clearDisp();
	menu();
}
