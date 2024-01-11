/*
 * clean_main.c
 *
 *  Created on: 15 gru 2022
 *      Author: Konrad
 */

#define ARM_MATH_CM4
#define BLOCK_SIZE 2048
#define FILTER_NUMBER 12

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
uint16_t samples[BLOCK_SIZE];
float32_t filterIn[BLOCK_SIZE];
uint16_t measured_us = 0;

float32_t filterState[FILTER_NUMBER][2];
float32_t filterCoefficients[FILTER_NUMBER][5] = {
	{1,0,-1,(float32_t)1.997720464600759804341123526683077216148,(float32_t)-0.997736103910341842748721319367177784443},
	{1,0,-1,(float32_t)1.995487280288827713903287985885981470346,(float32_t)-0.995547831259930848801786851254291832447},
	{1,0,-1,(float32_t)1.992054945031311419256780936848372220993,(float32_t)-0.992239234378425094540432382927974686027},
	{1,0,-1,(float32_t)1.986084374391128060111100239737424999475,(float32_t)-0.986632632240756035635342868772568181157},
	{1,0,-1,(float32_t)1.975250452882447271818477929627988487482,(float32_t)-0.976896080342992023659576261707115918398},
	{1,0,-1,(float32_t)1.954703773791649279445437059621326625347,(float32_t)-0.959741664255281956563692347117466852069},
	{1,0,-1,(float32_t)1.916819923240591805324584129266440868378,(float32_t)-0.931568042550358410558430932724149897695},
	{1,0,-1,(float32_t)1.844779386045683144956797150371130555868,(float32_t)-0.886302452742758317683069435588549822569},
	{1,0,-1,(float32_t)1.684341878847488871429050050210207700729,(float32_t)-0.807272535426271864444913717306917533278},
	{1,0,-1,(float32_t)1.349298830458679487875883751257788389921,(float32_t)-0.685985243898569851950242082239128649235},
	{1,0,-1,(float32_t)0.664827275766936565126741243147989735007,(float32_t)-0.50385547249973716699145143138593994081 },
	{1,0,-1,(float32_t)-0.484716934296551105187944585850345902145,(float32_t)-0.222624489976273709501697339874226599932}
};
float32_t gain[FILTER_NUMBER]={
	(float32_t)0.001131948044828745558731952769448980689 ,
	(float32_t)0.00222608437003468662140903688850812614  ,
	(float32_t)0.003880382810787397218632577278185635805 ,
	(float32_t)0.006683683879622037693479796871542930603 ,
	(float32_t)0.011551959828503766125606944115133956075 ,
	(float32_t)0.02012916787235918825160752021474763751  ,
	(float32_t)0.034215978724820739209633302380098029971 ,
	(float32_t)0.056848773628620730136162819690071046352 ,
	(float32_t)0.096363732286863901244089447573060169816 ,
	(float32_t)0.157007378050714963002576496364781633019 ,
	(float32_t)0.248072263750131360993123053049203008413 ,
	(float32_t)0.388687755011863145249151330062886700034 
};
float32_t filterOut[FILTER_NUMBER][BLOCK_SIZE];
arm_biquad_cascade_df2T_instance_f32 filter[FILTER_NUMBER];
float32_t bar_values[FILTER_NUMBER];

typedef struct menu_option{
	char * prompt;
	void (*func)(void);
}menu_option;

void initializeFilters(uint8_t numberOfFilters){
	for(uint8_t i=0;i<numberOfFilters;i++){
		arm_biquad_cascade_df2T_init_f32(&filter[i],1,filterCoefficients[i],filterState[i]);
	}
}

void processBand(uint8_t bandNumber){
	if(bandNumber>FILTER_NUMBER-1) return;
	arm_biquad_cascade_df2T_f32(&filter[bandNumber],filterIn,filterOut[bandNumber],BLOCK_SIZE);
	uint16_t max_value = filterOut[bandNumber][0]; 
	for(uint16_t i=1;i<BLOCK_SIZE;i++){
		if(filterOut[bandNumber][i]>max_value) max_value = filterOut[bandNumber][i];
	}
	bar_values[bandNumber] = max_value * gain[bandNumber];
}

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

void testAllDac(){
	dprint("Testing all DACs    sequentially");
	while(button() == 1);
	switchTube(1);
	for(uint8_t i=0;i<12;i++){
		for(uint8_t j=0;j<220;j++){
			setBar(i,j);
			HAL_Delay(5);
			if(button()==1){
				switchTube(0);
				while(button() == 1);
				return;
			} 
		}
		for(uint8_t j=220;j>5;j--){
			setBar(i,j);
			HAL_Delay(5);
			if(button()==1){
				switchTube(0);
				while(button() == 1);
				return;
			} 
		}
	}
	for(uint8_t i=0;i<12;i++) setBar(i,220);
	while(button()==0);
	switchTube(0);
	while(button() == 1);
}

void TestFilterOut(){
	stan_enc = 0 ;
	while(button() == 1);
	while(button() == 0){
	
		UpdateEnc(0, FILTER_NUMBER-1, &stan_enc, false);
		dprint("Filter: %.3fV CH:%2dtime[us]:%d",bar_values[stan_enc],stan_enc,measured_us);
		HAL_Delay(100);
	}
	while(button() == 1);
}

void display_bars(){
	switchTube(1);
	for(uint8_t i=0;i<15;i++){
		for(uint8_t j=0;j<FILTER_NUMBER;j++){
			setBarInv(j,i);
			HAL_Delay(1);
		}
	}
	float32_t smooth_bar_values[FILTER_NUMBER];
	dprint("   Nixie spectrum         analyzer      ");
	while(button() == 1);
	while(button() == 0){
		for(uint8_t i=0;i<FILTER_NUMBER;i++){
			if(button() == 1) break;
			if(bar_values[i]>smooth_bar_values[i]){
				smooth_bar_values[i] += 0.008;
			}else if(bar_values[i]<smooth_bar_values[i]){
				smooth_bar_values[i] -= 0.008;
			}
			setBarInv(i,(uint8_t)(smooth_bar_values[i]*128.125)+15);
		}
		HAL_Delay(1);
	}
	while(button() == 1);
	switchTube(0);
}

void menu(){

	struct menu_option options[] = {
			{"Display bar",line},
			{"Set bar character",change_char},
			{"Test tube power",testOutput},
			{"Test DAC", testDAC},
			{"Test filter output",TestFilterOut},
			{"Test all DAC",testAllDac},
			{"Spectrum",display_bars}
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
	//Copy DMA buffer to avoid processing overwritten data, at the same time convert measurements to voltage in float and cut DC offset

	TIM1 -> CNT = 0;
	HAL_TIM_Base_Start(&htim1);

	for(uint16_t i=0;i<BLOCK_SIZE;i++){
		filterIn[i]=(samples[i]-2048)*(3.3/4096);
	}

	for(uint8_t i=0;i<FILTER_NUMBER;i++){
		processBand(i);
	}

	HAL_TIM_Base_Stop(&htim1);
	measured_us = TIM1 -> CNT;

}

void clean_main(){
	//Start Encoder Counter
	HAL_TIM_Base_Start(&htim2);
	//Start ADC in DMA mode
	HAL_ADC_Start_DMA(&hadc1, samples, BLOCK_SIZE);
	//Set 0 on all DACs channels
	for(uint8_t i = 0;i<16;i++) setBar(i,0);
	//Initialize digital peak filters
	initializeFilters(FILTER_NUMBER);
	//Delay to let display start properly
	HAL_Delay(500);
}

void clean_loop(){
	clearDisp();
	menu();
}
