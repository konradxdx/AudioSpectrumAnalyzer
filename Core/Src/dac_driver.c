/*
 * dac_driver.c
 *
 *  Created on: 17 gru 2022
 *      Author: Konrad
 */

#include "dac_driver.h"
#include "spi.h"


void dac_write(uint8_t channel, uint8_t value, uint8_t dac){
	HAL_GPIO_WritePin(LAT0_GPIO_Port, LAT0_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LAT1_GPIO_Port, LAT1_Pin, GPIO_PIN_RESET);
	
	if(dac == 0)
		HAL_GPIO_WritePin(CS0_GPIO_Port, CS0_Pin, GPIO_PIN_RESET);
	else if(dac == 1)
		HAL_GPIO_WritePin(CS1_GPIO_Port, CS1_Pin, GPIO_PIN_RESET);
	else if(dac == 2)
		HAL_GPIO_WritePin(CS2_GPIO_Port, CS2_Pin, GPIO_PIN_RESET);
	else if(dac == 3)
		HAL_GPIO_WritePin(CS3_GPIO_Port, CS3_Pin, GPIO_PIN_RESET);
	else
		return;
		
	uint8_t toSend[3] = {0,0,0};

	switch(channel){
		case 0:
		toSend[0]=0b00000000;
		break;
		case 1:
		toSend[0]=0b00010000;
		break;
		case 2:
		toSend[0]=0b00011000;
		break;
		case 3:
		toSend[0]=0b00001000;
		break;
		default:
		return 0;
	}

	toSend[1]=value;
	toSend[2]=value;

	HAL_SPI_Transmit(&hspi1, toSend, 3, 100);

	if(dac == 0)
		HAL_GPIO_WritePin(CS0_GPIO_Port, CS0_Pin, GPIO_PIN_SET);
	else if(dac == 1)
		HAL_GPIO_WritePin(CS1_GPIO_Port, CS1_Pin, GPIO_PIN_SET);
	else if(dac == 2)
		HAL_GPIO_WritePin(CS2_GPIO_Port, CS2_Pin, GPIO_PIN_SET);
	else if(dac == 3)
		HAL_GPIO_WritePin(CS3_GPIO_Port, CS3_Pin, GPIO_PIN_SET);
	else
		return;

	HAL_GPIO_WritePin(LAT0_GPIO_Port, LAT0_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LAT1_GPIO_Port, LAT1_Pin, GPIO_PIN_SET);

	return;
}

void setBar(uint8_t barNumber,uint8_t value){
	if(barNumber<4){
		dac_write(barNumber,value,0);
	}else if(barNumber<8){
		dac_write(barNumber-4,value,1);
	}else if(barNumber<12){
		dac_write(barNumber-8,value,2);	
	}else if(barNumber<16){
		dac_write(barNumber-12,value,3);
	}else{
		return;
	}
}

void setBarInv(uint8_t barNumber,uint8_t value){
	setBar(11-barNumber,value);
}