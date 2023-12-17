/*
 * clean_main.c
 *
 *  Created on: 15 gru 2022
 *      Author: Konrad
 */

#define ARM_MATH_CM4

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdbool.h>
#include "arm_math.h"
#include "enc_disp.h"
#include "dac_driver.h"

float32_t fft_in_buf[512];
float32_t fft_out_buf[512];
arm_rfft_fast_instance_f32 fft_handler;

int16_t stan_enc = 20;
uint8_t znak = 127;
char pasek[20];
uint32_t samples[2];

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

void switchTube(bool state){
	HAL_GPIO_WritePin(EN_TUBE_GPIO_Port,EN_TUBE_Pin,state);
}

void testOutput(void){
	clearDisp();
	while(button() == 1);
	while(button() == 0){
		HAL_Delay(1000);
		switchTube(1);
		dprint("Out:1");
		HAL_Delay(1000);
		switchTube(0);
		dprint("Out:0");
	}
	while(button() == 1);
}

void testDAC(){
	HAL_Delay(500);
	clearDisp();
	stan_enc = 0;
	while(button() == 0){
		UpdateEnc(0, 3, &stan_enc, false);
		dprint("DAC Channel:%1d",stan_enc);
		HAL_Delay(50);
	}
	int channel = stan_enc;
	while(button() == 1);
	stan_enc = 0;
	while(button() == 0){
		UpdateEnc(0, 3, &stan_enc, false);
		dprint("Number of DAC:%1d",stan_enc);
		HAL_Delay(50);
	}
	int dac_number = stan_enc;
	while(button() == 1);
	stan_enc = 0;
	HAL_GPIO_WritePin(EN_TUBE_GPIO_Port,EN_TUBE_Pin,1);
	while(button() == 0){
		UpdateEnc(0, 220, &stan_enc, true);
		dprint("DAC output:%3d",stan_enc);
		dac_write(channel,stan_enc,dac_number);
		HAL_Delay(50);	
	}
	while(button() == 1);
	HAL_GPIO_WritePin(EN_TUBE_GPIO_Port,EN_TUBE_Pin,0);

}

void menu(){

	struct menu_option options[] = {
			{"Display bar",line},
			{"Set bar character",change_char},
			{"Test tube power",testOutput},
			{"Test DAC", testDAC}
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
	HAL_ADC_Start_DMA(&hadc1, samples, 2);
	//Set All DACs to 0 !!!
	for(uint8_t i = 0;i<4;i++){
		for(uint8_t j = 0;j<4;j++) dac_write(i,0,j);
	}
	//arm_rfft_fast_init_f32(&fft_handler,512);
	//arm_rfft_fast_f32(&fft_handler, fft_in_buf ,fft_out_buf,0);
	HAL_Delay(1000);
}

void clean_loop(){
	clearDisp();
	menu();
}
