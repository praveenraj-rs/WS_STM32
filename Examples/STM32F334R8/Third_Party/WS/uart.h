#ifndef UART_H_
#define UART_H_

#include "stm32f4xx.h"

/* Initialize UART2 (PA2 TX, PA3 RX) */
void UART2_Init(void);

/* Send single character */
void UART2_SendChar(char c);

/* Send string */
void UART2_SendString(char *s);

/* Send Integer Number */
void UART2_SendInt(int num);

#endif /* UART_H_ */
