#include "uart.h"
#include <stdint.h>
#include <stdio.h>

void UART2_Init(void)
{
    // UART2 Config
    // Tx -> PA2
    // Rx -> PA3

    // Buard Rate Configuration
    uint16_t brr = 0x0683;  //9600   @16MHz
    // uint16_t brr = 0x0341;  //19200  @16MHz
    // uint16_t brr = 0x01A1;  //38400  @16MHz
    // uint16_t brr = 0x0116;  //57600  @16MHz
    // uint16_t brr = 0x008B;  //115200 @16MHz

    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 -> TX, PA3 -> RX (Alternate Function) */
    GPIOA->MODER &= ~((3<<(2*2)) | (3<<(3*2)));
    GPIOA->MODER |=  ((2<<(2*2)) | (2<<(3*2)));

    /* AF7 for USART2 */
    GPIOA->AFR[0] &= ~((0xF<<(2*4)) | (0xF<<(3*4)));
    GPIOA->AFR[0] |=  ((7<<(2*4)) | (7<<(3*4)));

    /* Baud rate 9600 @16MHz */
    USART2->BRR = brr;

    /* Enable TX + UART */
    USART2->CR1 = USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;
}

void UART2_SendChar(char c)
{
    while(!(USART2->SR & USART_SR_TXE));
    USART2->DR = c;
}

void UART2_SendString(char *s)
{
    while(*s)
    {
        UART2_SendChar(*s++);
    }
}

void UART2_SendInt(int num)
{
    char buffer[20];
    sprintf(buffer, "%d", num);
    UART2_SendString(buffer);
}
