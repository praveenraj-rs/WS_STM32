// Blink LED (Built-in LED)

#include "stm32f4xx.h"

#define SysClk 16000000U 	// 16MHz HSI system clock frequency

#define LED 13U 			// PC13 Built-in LED
#define BTN 0U 				// PA0 push-BTN active low

void LED_GPIO_Init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 	// Enable GPIOC clock
	GPIOC->MODER &= ~(3U << (LED * 2));	// Clear register
	GPIOC->MODER |=  (1U << (LED * 2));  	// 01: Output mode
	GPIOC->OTYPER &= ~(1U << LED);		// Push-pull
	GPIOC->PUPDR &= ~(3U << (LED * 2));	// No pull-up or pull-down
	GPIOC->ODR |= (1<<LED);				// LED off
}

void BTN_GPIO_Init(void)
{
   RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;		// Enable GPIOA clock
   GPIOA->MODER &= ~(3U << (BTN * 2));    	// 00: Input mode
   GPIOA->PUPDR &= ~(3U << (BTN * 2));    	// Clear register
   GPIOA->PUPDR |= (1U << (BTN * 2));    	// Pull-up configuration
}

void TIM3_Delay_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;			// Enable TIM3 Clock
    uint32_t prescaler = (SysClk / 1000) - 1;	// Timer clock = 1 KHz or 1ms
    TIM3->PSC = prescaler;						// Pre-scaler update
}

void TIM3_Delay_ms(uint16_t delay_ms)
{
    TIM3->ARR = delay_ms - 1;   			// Auto-reload 1 second delay (1ms * 999)
    TIM3->CNT = 0;							// Reset the counter
    TIM3->EGR |= (1 << 0);  				// Event generation

    TIM3->SR &= ~ (1 << 0);  				// Clear update interrupt flag
    TIM3->CR1 |= (1 << 0); 					// Enable Timer

    while (!(TIM3->SR & (1 << 0))){} 		// Until UIF NEQ 1
    TIM3->SR &= ~(1 << 0); 					//UIF is cleared manually
    TIM3->CR1 &= ~(1 << 0); 				//CEN is cleared
}

int main(void)
{
	// Initialization
	LED_GPIO_Init();		// Built-in LED initialization
	TIM3_Delay_Init(); 		// Timer3 Delay initialization

	while (1)
	{
		GPIOC->ODR ^= (1<<LED);	    // LED OFF
		TIM3_Delay_ms(500);			// Delay
	}
}
