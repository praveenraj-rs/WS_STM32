#include "stm32f4xx.h"

#define SysClk 16000000U   // 16 MHz HSI clock
#define LED 12U            // PD12 Green LED

void LED_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;   // Enable GPIOD clock

    GPIOD->MODER &= ~(3U << (LED * 2));    // Clear mode bits
    GPIOD->MODER |=  (1U << (LED * 2));    // Set as output mode (01)

    GPIOD->OTYPER &= ~(1U << LED);         // Push-pull
    GPIOD->PUPDR &= ~(3U << (LED * 2));    // No pull-up/pull-down
}

void TIM3_Delay_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;    // Enable TIM3 clock

    uint32_t prescaler = (SysClk / 1000) - 1;  // 1 ms timer
    TIM3->PSC = prescaler;
}

void TIM3_Delay_ms(uint16_t delay_ms)
{
    TIM3->ARR = delay_ms - 1;
    TIM3->CNT = 0;
    TIM3->EGR |= 1;        // Update registers

    TIM3->SR &= ~(1 << 0); // Clear UIF
    TIM3->CR1 |= 1;        // Start timer

    while(!(TIM3->SR & 1)); // Wait for overflow

    TIM3->SR &= ~(1 << 0); // Clear flag
    TIM3->CR1 &= ~(1 << 0);// Stop timer
}

int main(void)
{
    LED_GPIO_Init();       // Initialize PD12
    TIM3_Delay_Init();     // Timer delay init

    while (1)
    {
        GPIOD->ODR ^= (1 << LED);  // Toggle PD12
        TIM3_Delay_ms(500);        // 500 ms delay
    }
}
