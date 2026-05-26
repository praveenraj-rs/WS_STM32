#include "stm32f334x8.h"

void delay(void)
{
    for (volatile uint32_t i = 0; i < 500000; i++);
}

int main(void)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Set PA5 as output mode
       MODER bits for PA5 = 01
    */
    GPIOA->MODER &= ~(3U << (5 * 2));   // Clear mode bits
    GPIOA->MODER |=  (1U << (5 * 2));   // Set as output

    while (1)
    {
        GPIOA->ODR ^= (1U << 5);   // Toggle PA5
        delay();
    }
}
