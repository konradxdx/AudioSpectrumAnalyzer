/*
 * dac_driver.h
 *
 *  Created on: 17 gru 2022
 *      Author: Konrad
 */

#ifndef INC_DAC_DRIVER_H_
#define INC_DAC_DRIVER_H_



#endif /* INC_DAC_DRIVER_H_ */

#include <stdint.h>

void dac_write(uint8_t channel, uint8_t value, uint8_t dac);
void setBar(uint8_t barNumber,uint8_t value);
void setBarInv(uint8_t barNumber,uint8_t value);