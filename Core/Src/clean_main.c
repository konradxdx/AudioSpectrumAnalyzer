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

//global variables 
int16_t stan_enc = 0;
uint16_t measured_us = 0;
float32_t offset = 0.11707317;
uint8_t nixie_state = 0;

//settings
uint8_t operating_mode = 0; //0-average two channels 1-left 2-right
uint8_t step_size = 5;		//0-10 range
uint8_t default_screen = 0; //0-blank screen 1-default text 2-clock
uint16_t timeout = 10;		//time to wait before turning off lamps when there is no audio signal

//filter and display variables and arrays
float32_t filterState[FILTER_NUMBER][2];
float32_t filterCoefficients[FILTER_NUMBER][5] = {
	{1,0,-1,(float32_t)1.997391684898714547813369790674187242985,(float32_t)-0.997412123994309562924343026679707691073},
	{1,0,-1,(float32_t)1.995413068693758118499204101681243628263,(float32_t)-0.99547560251750877302612252606195397675 },
	{1,0,-1,(float32_t)1.991923221573191238320532647776417434216,(float32_t)-0.992113540629178114471642402349971234798},
	{1,0,-1,(float32_t)1.985850578617751827081860938051249831915,(float32_t)-0.986416748379982721850467441981891170144},
	{1,0,-1,(float32_t)1.974825534194301290824569150572642683983,(float32_t)-0.976524785603339440243075841863173991442},
	{1,0,-1,(float32_t)1.953898987653213570325760883861221373081,(float32_t)-0.959100266280781599270710557902930304408},
	{1,0,-1,(float32_t)1.915270578384612232270001186407171189785,(float32_t)-0.930493093033518792189795476588187739253},
	{1,0,-1,(float32_t)1.841716645687592857072445440280716866255,(float32_t)-0.884556143060444721726298666908405721188},
	{1,0,-1,(float32_t)1.677710466777201592236679061898030340672,(float32_t)-0.804423420295921731693056244694162160158},
	{1,0,-1,(float32_t)1.335167626320647338289404615352395921946,(float32_t)-0.681591652234631695250755001325160264969},
	{1,0,-1,(float32_t)0.637552258449676889640045374108012765646,(float32_t)-0.49737732924063193795660708929062820971 },
	{1,0,-1,(float32_t)-0.516464981783446352991973071766551584005,(float32_t)-0.212987554855081140381400928163202479482}
};
float32_t gain[FILTER_NUMBER]={
	(float32_t)0.001293938002845385071282180433627218008 ,
	(float32_t)0.00226219874124578002039243074250407517  ,
	(float32_t)0.003943229685410942764178798825014382601 ,
	(float32_t)0.006791625810008472541312585235573351383 ,
	(float32_t)0.011737607198330390900764541584067046642 ,
	(float32_t)0.020449866859609366898098414822015911341 ,
	(float32_t)0.034753453483240659416253492963733151555 ,
	(float32_t)0.057721928469777639136850666545797139406 ,
	(float32_t)0.097788289852039023131169415137264877558 ,
	(float32_t)0.159204173882684152374622499337419867516 ,
	(float32_t)0.251311335379683975510545224096858873963 ,
	(float32_t)0.393506222572459485320450767176225781441 
};
float32_t filterOut[FILTER_NUMBER][BLOCK_SIZE];
arm_biquad_cascade_df2T_instance_f32 filter[FILTER_NUMBER];
//raw adc sample array for DMA to write into
uint16_t samples[BLOCK_SIZE*2];
//array with selected left right or calculated avg. values to feed into filter
float32_t filterIn[BLOCK_SIZE];
//raw values to display and smoothed ones
float32_t bar_values[FILTER_NUMBER];
float32_t smooth_bar_values[FILTER_NUMBER];

//menu option typedef 
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
	//return if wrong bar number
	if(bandNumber>FILTER_NUMBER-1) return; 
	//calculate filter output
	arm_biquad_cascade_df2T_f32(&filter[bandNumber],filterIn,filterOut[bandNumber],BLOCK_SIZE);
	//set first value from filter output as max for start 
	uint16_t max_value = filterOut[bandNumber][0]; 
	//search for maximum value in array
	for(uint16_t i=1;i<BLOCK_SIZE;i++){
		if(filterOut[bandNumber][i]>max_value) max_value = filterOut[bandNumber][i];
	}
	//calculate bar_values with gain correction
	bar_values[bandNumber] = max_value * gain[bandNumber];
}

void switchTube(bool state){
	HAL_GPIO_WritePin(EN_TUBE_GPIO_Port,EN_TUBE_Pin,state);
}

void TestFilterOut(){
	stan_enc = 0 ;
	while(button() == 0){
	
		UpdateEnc(0, FILTER_NUMBER-1, &stan_enc, false);
		dprint("Filter: %.3fV CH:%2dtime[us]:%d",bar_values[stan_enc],stan_enc,measured_us);
		HAL_Delay(100);
	}
}

void changeSamplingMode(){
	stan_enc = 0 ;
	while(button() == 0){
	
		UpdateEnc(0, 2, &stan_enc, false);
		operating_mode = stan_enc;
		if(stan_enc == 0)	dprint("Sampling mode:      avg. of two channels");
		if(stan_enc == 1)	dprint("Sampling mode:      left channel only   "); 
		if(stan_enc == 2)	dprint("Sampling mode:      right channel only  ");
		HAL_Delay(100);
	}
}

void changeIncStep(){
	stan_enc = 1 ;
	while(button() == 0){
		UpdateEnc(1, 10, &stan_enc, false);
		step_size = stan_enc;
		dprint("Step value: %d",step_size);
		HAL_Delay(100);
	}	
}

void changeTimeout(){
	stan_enc = 1 ;
	while(button() == 0){
		UpdateEnc(1, 180, &stan_enc, false);
		timeout = stan_enc*10;
		dprint("Timeout in seconds: %d",timeout);
		HAL_Delay(100);
	}	
}

void changeDefaultScreen(){
	stan_enc = 0 ;
	while(button() == 0){
		UpdateEnc(0, 2, &stan_enc, false);
		default_screen = stan_enc;
		if(stan_enc == 0)	dprint("Default screen:     blank               ");
		if(stan_enc == 1)	dprint("Default screen:     device name         "); 
		if(stan_enc == 2)	dprint("Default screen:     clock               ");
		HAL_Delay(100);
	}	
}

void enable_nixie(){
	if(nixie_state == 0){
		for(uint8_t i=0; i<FILTER_NUMBER;i++){
			setBarInv(i,0);
			smooth_bar_values[i] = 0;
		}
		switchTube(1);
		for(uint8_t i=0;i<15;i++){
			for(uint8_t j=0;j<FILTER_NUMBER;j++){
				setBarInv(j,i);
			}
		HAL_Delay(100);
		}
		HAL_TIM_Base_Start_IT(&htim4);	
		nixie_state = 1;	
	}
}

void disable_nixie(){
	HAL_TIM_Base_Stop_IT(&htim4);
	switchTube(0);
	nixie_state = 0;
}

void testDAC(){
	for(uint8_t i=0; i<FILTER_NUMBER;i++){
		setBarInv(i,0);
		smooth_bar_values[i] = 0;
	}
	HAL_TIM_Base_Stop_IT(&htim4);
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
	while(button() == 0){
		UpdateEnc(0, 255, &stan_enc, true);
		dprint("DAC output:%3d",stan_enc);
		dac_write(channel,stan_enc,dac_number);
		HAL_Delay(50);	
	}
	nixie_state = 0;
	enable_nixie();
}

void testAllDac(){
	for(uint8_t i=0; i<FILTER_NUMBER;i++){
		setBarInv(i,0);
		smooth_bar_values[i] = 0;
	}
	HAL_TIM_Base_Stop_IT(&htim4);
	dprint("Testing all DACs    sequentially");
	for(uint8_t i=0;i<12;i++){
		for(uint8_t j=0;j<220;j++){
			setBarInv(i,j);
			HAL_Delay(5);
			if(button()==1){
				nixie_state = 0;
				enable_nixie();
				return;
			} 
		}
		for(uint8_t j=220;j>5;j--){
			setBarInv(i,j);
			HAL_Delay(5);
			if(button()==1){
				nixie_state = 0;
				enable_nixie();
				return;
			} 
		}
	}
	for(uint8_t i=0;i<12;i++) setBar(i,220);
	while(button()==0);
	nixie_state = 0;
	enable_nixie();
}

void defaultScreen(){
	while(button() == 1);

	//stay with clear display
	if(default_screen == 0){
		clearDisp();
	}
	//stay with default text
	if(default_screen == 1){
		dprint("   Nixie spectrum          analyzer      ");
	}

	while(button()==0){
		for(uint8_t i=0;i<timeout*2;i++){
			if(button() == 1){
				enable_nixie();
				while(button() == 1);
				return;
			}
			float avg_lvl = 0;
			for(uint8_t j=0;j<FILTER_NUMBER;j++) avg_lvl += bar_values[j];
			avg_lvl = avg_lvl/FILTER_NUMBER;

			if(avg_lvl > 0.05){
				i=0;
				enable_nixie();
			}
			if(default_screen == 2){
				//wyswietl zegarek
			}
			HAL_Delay(500);
		}
		disable_nixie();
	}

	while(button() == 1);
}

void exitMenu(){
	return;
}

void menu(){

	struct menu_option options[] = {
		{"Bar increment step",changeIncStep},
		{"Sampling mode",changeSamplingMode},
		{"Change timeout",changeTimeout},
		{"Change default      screen",changeDefaultScreen},		
		{"Test DAC", testDAC},
		{"Test all DAC",testAllDac},
		{"Test filter output",TestFilterOut},
		{"Turn nixie off",disable_nixie},
		{"Turn nixie on",enable_nixie},
		{"Exit",exitMenu}
	};

	int16_t position = 0;
	uint8_t options_count = sizeof(options)/sizeof(options[0]);

	while(button()==0){
		UpdateEnc(0,options_count-1, &position, false);
		dprint("%d.%s",position+1, options[position].prompt);
		HAL_Delay(50);
	}
	while(button() == 1);
	options[position].func();
	while(button() == 1);
}

void conversion_complete(ADC_HandleTypeDef* hadc) {

	TIM1 -> CNT = 0;
	HAL_TIM_Base_Start(&htim1);
	//Copy DMA buffer to avoid processing overwritten data, at the same time convert measurements to voltage in float and cut DC offset
	
	if(operating_mode == 0){	//sum of 2 channels
		for(uint16_t i=0;i<BLOCK_SIZE;i++){
		filterIn[i]= ((samples[i*2]-2048)*(3.3/4096) + (samples[(i*2)+1]-2048)*(3.3/4096))/2;
		}
	}else if(operating_mode == 1){	//left
		for(uint16_t i=0;i<BLOCK_SIZE;i++){			
			filterIn[i]=(samples[i*2]-2048)*(3.3/4096);
		}
	}else if(operating_mode == 2){	//right
		for(uint16_t i=0;i<BLOCK_SIZE;i++){	
			filterIn[i]=(samples[(i*2)+1]-2048)*(3.3/4096);
		}
	}

	for(uint8_t i=0;i<FILTER_NUMBER;i++){
		processBand(i);
	}

	HAL_TIM_Base_Stop(&htim1);
	measured_us = TIM1 -> CNT;

}

void cyclic_interrupt(TIM_HandleTypeDef* htim){
	for(uint8_t i=0;i<FILTER_NUMBER;i++){
		if((bar_values[i]+offset)>smooth_bar_values[i]){
			smooth_bar_values[i] += (0.001*step_size);
		}else if((bar_values[i]+offset)<smooth_bar_values[i]){
			smooth_bar_values[i] -= (0.001*step_size);
		}
		setBarInv(i,(uint8_t)(smooth_bar_values[i]*128.125));
	}
}

void clean_main(){
	//callback registration
  	HAL_ADC_RegisterCallback(&hadc1,HAL_ADC_CONVERSION_COMPLETE_CB_ID,conversion_complete);
  	HAL_TIM_RegisterCallback(&htim4, HAL_TIM_PERIOD_ELAPSED_CB_ID, cyclic_interrupt);	
	//Set 0 on all DACs channels
	for(uint8_t i = 0;i<16;i++) setBar(i,0);
	//Initialize digital peak filters
	initializeFilters(FILTER_NUMBER);
	//Start ADC in DMA mode
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)samples, BLOCK_SIZE*2);
	//Start Encoder Counter
	HAL_TIM_Base_Start(&htim2);
	//Start sampling timer
	HAL_TIM_Base_Start(&htim3);
	//Start cyclic interrupt timer
	HAL_TIM_Base_Start_IT(&htim4);
	//Delay to let display start properly
	dprint("   Nixie spectrum       analyzer V1.0   ");
	HAL_Delay(2000);
	dprint("       Autor:        Konrad Hryniewicki ");
	HAL_Delay(2000);
	//Proper start of nixie tubes
	enable_nixie();

}

void clean_loop(){
	defaultScreen();
	menu();
}
