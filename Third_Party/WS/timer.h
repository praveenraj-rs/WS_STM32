#ifndef TIMER_H_
#define TIMER_H_

#include "stm32f4xx.h"

/* Initialize TIM3 for delay */
void TIM3_Delay_Init(void);

/* Blocking delay in milliseconds */
void TIM3_Delay_ms(uint16_t ms);

#endif /* TIMER_H_ */
