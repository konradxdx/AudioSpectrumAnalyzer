/*
 * disp.h
 *
 *  Created on: Dec 10, 2022
 *      Author: Konrad
 */

#ifndef INC_DISP_H_
#define INC_DISP_H_

#include <stdint.h>
#include <stdbool.h>

#endif /* INC_DISP_H_ */

void dprint(const char *fmt, ...);
void clearDisp(void);
void UpdateEnc(uint8_t min, uint8_t max, int16_t* enc_state, bool stopAtBorder);
bool button(void);
