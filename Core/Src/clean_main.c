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




int16_t stan_enc = 20;
uint8_t znak = 127;
char pasek[20];
uint16_t samples[2048];
float32_t filterIn[2048];

//filter 1khz test
float32_t maxValue = 0;
uint32_t measured_us = 0;
float32_t filterState[2];
float32_t filterCoefficients[5] = {1,0,-1,(float32_t)1.944915703229297632148586671974044293165,(float32_t)-0.959879993036016454510672701871953904629};
float32_t filterOut[2048];
arm_biquad_cascade_df2T_instance_f32 filter1;



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
		UpdateEnc(0, 255, &stan_enc, true);
		dprint("DAC output:%3d",stan_enc);
		dac_write(channel,stan_enc,dac_number);
		HAL_Delay(50);	
	}
	while(button() == 1);
	HAL_GPIO_WritePin(EN_TUBE_GPIO_Port,EN_TUBE_Pin,0);

}

void TestFilterOut(){
	while(button() == 1);
	while(button() == 0){
		dprint("Filter: %.3fV      time[us]:%d",maxValue,measured_us);
		HAL_Delay(100);
	}
	while(button() == 1);
}

void menu(){

	struct menu_option options[] = {
			{"Display bar",line},
			{"Set bar character",change_char},
			{"Test tube power",testOutput},
			{"Test DAC", testDAC},
			{"Test filter output",TestFilterOut}
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

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	for(uint16_t i=0;i<2048;i++){
		filterIn[i]=(samples[i]-2048)*(3.3/4096);
	}

	TIM1 -> CNT = 0;
	HAL_TIM_Base_Start(&htim1);

	arm_biquad_cascade_df2T_f32(&filter1,filterIn,filterOut,2048);
	float32_t maxValueInt = filterOut[0]; 
	for(uint16_t i=1;i<2048;i++){
		if(filterOut[i]>maxValueInt) maxValueInt = filterOut[i];
	}
	maxValueInt=maxValueInt*0.02;
	if(maxValueInt<0.001) maxValueInt=0.001;
	dac_write(0,(uint8_t)(maxValueInt),0);

	HAL_TIM_Base_Stop(&htim1);
	measured_us = TIM1 -> CNT;
	maxValue = maxValueInt;
}

void clean_main(){
	HAL_TIM_Base_Start(&htim2);
	HAL_ADC_Start_DMA(&hadc1, samples, 2048);
	
	//Set All DACs to 0 !!!
	for(uint8_t i = 0;i<4;i++){
		for(uint8_t j = 0;j<4;j++) dac_write(i,0,j);
	}
	
	arm_biquad_cascade_df2T_init_f32(&filter1,1,filterCoefficients,filterState);

	HAL_Delay(1000);
}

void clean_loop(){
	clearDisp();
	menu();
}
