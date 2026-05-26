#include "stm32f4xx.h"

#define SYS_CLK 16000000U

/* ================= TIMER DELAY ================= */

void TIM3_Delay_ms(uint16_t ms)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = (SYS_CLK/1000) - 1;
    TIM3->ARR = ms - 1;
    TIM3->CNT = 0;

    TIM3->SR &= ~TIM_SR_UIF;
    TIM3->CR1 |= TIM_CR1_CEN;

    while(!(TIM3->SR & TIM_SR_UIF));

    TIM3->CR1 &= ~TIM_CR1_CEN;
}

/* ================= UART2 INIT ================= */

void UART2_Init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 TX , PA3 RX */

    GPIOA->MODER &= ~((3<<(2*2)) | (3<<(3*2)));
    GPIOA->MODER |=  ((2<<(2*2)) | (2<<(3*2)));

    GPIOA->AFR[0] &= ~((0xF<<(2*4)) | (0xF<<(3*4)));
    GPIOA->AFR[0] |=  ((7<<(2*4)) | (7<<(3*4)));

    USART2->BRR = 0x0683;   // 9600 baud @16MHz

    USART2->CR1 = USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;
}

/* ================= UART SEND ================= */

void UART_SendChar(char c)
{
    while(!(USART2->SR & USART_SR_TXE));
    USART2->DR = c;
}

void UART_SendString(char *s)
{
    while(*s)
    {
        UART_SendChar(*s++);
    }
}

/* ================= MAIN ================= */

int main()
{
    UART2_Init();

    while(1)
    {
        UART_SendString("Hello World\r\n");
        TIM3_Delay_ms(1000);
    }
}
