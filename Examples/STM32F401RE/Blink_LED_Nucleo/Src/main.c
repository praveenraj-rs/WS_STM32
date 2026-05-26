// Blink LED (User LED - Nucleo STM32F401RE)

#include "stm32f4xx.h"

#define SysClk 16000000U     // 16 MHz HSI system clock frequency

#define LED 5U               // PA5 User LED

void LED_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // Enable GPIOA clock

    GPIOA->MODER &= ~(3U << (LED * 2));    // Clear mode bits
    GPIOA->MODER |=  (1U << (LED * 2));    // 01: Output mode

    GPIOA->OTYPER &= ~(1U << LED);         // Push-pull

    GPIOA->PUPDR &= ~(3U << (LED * 2));    // No pull-up/pull-down

    GPIOA->ODR &= ~(1U << LED);            // LED OFF initially
}

void TIM3_Delay_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;    // Enable TIM3 clock

    uint32_t prescaler = (SysClk / 1000) - 1; // Timer clock = 1 kHz (1 ms)
    TIM3->PSC = prescaler;
}

void TIM3_Delay_ms(uint16_t delay_ms)
{
    TIM3->ARR = delay_ms - 1;
    TIM3->CNT = 0;

    TIM3->EGR |= (1 << 0);      // Update event
    TIM3->SR &= ~(1 << 0);      // Clear UIF
    TIM3->CR1 |= (1 << 0);      // Enable timer

    while (!(TIM3->SR & (1 << 0)));  // Wait until update flag

    TIM3->SR &= ~(1 << 0);      // Clear UIF
    TIM3->CR1 &= ~(1 << 0);     // Stop timer
}

int main(void)
{
    LED_GPIO_Init();        // Initialize PA5 LED
    TIM3_Delay_Init();      // Initialize TIM3 delay

    while (1)
    {
        GPIOA->ODR ^= (1 << LED);   // Toggle LED
        TIM3_Delay_ms(500);         // 500 ms delay
    }
}
