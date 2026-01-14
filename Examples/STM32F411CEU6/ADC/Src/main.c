#include "stm32f4xx.h"
#include <stdint.h>
#include <math.h>

#define SysClk 16000000U
#define POT_PIN 0U 					// PA0 ADC for 10kohm Potentiometer

volatile uint16_t adc_value;

void TIM3_Init()
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
	 TIM3->CR1 |= (1 << 0); 				// Enable Timer

	 while (!(TIM3->SR & (1 << 0))){} 		// Until UIF NEQ 1
	 TIM3->SR &= ~(1 << 0); 				//UIF is cleared manually
	 TIM3->CR1 &= ~(1 << 0); 				//CEN is cleared
}

void POT_ADC_Init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;	      	// GPIOA clock enable
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;		      	// ADC1 clock enable

	// Configure POT (PA0) as analog
	GPIOA->MODER &= ~(0x3 << (POT_PIN * 2));	  	// Clear bits to configuration
	GPIOA->MODER |=  (0x3 << (POT_PIN * 2));	  	// Analog mode

	// Sample Time 84 cycle
	ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;					// Clear bits to configuration
	ADC1->SMPR2 |= (0x4 << ADC_SMPR2_SMP0_Pos); 	// Set 84 cycle for channel 0

	// Regular sequence
	ADC1->SQR3 &= ~ADC_SQR3_SQ1;			       	// Clear bits to configure
	ADC1->SQR3 |= (0 << ADC_SQR3_SQ1_Pos); 	    	// Channel 0 in SQ1

	// ADC Control Configuration
	ADC1->CR2 |= ADC_CR2_ADON; 				    	// ADC ON

	for (int i = 0; i < 1000; i++){}
}

int main(void)
{
	TIM3_Init();
	POT_ADC_Init();

	while (1)
	{
		uint32_t sum = 0;

		for (int i = 0; i < 16; i++)
		{
			ADC1->CR2 |= ADC_CR2_SWSTART;		// Software start conversion
			while (!(ADC1->SR & ADC_SR_EOC)){}	// Wait for EOC
			sum += ADC1->DR & 0xFFF;			// Read data 12bit (reading DR clears EOC)
		}

		adc_value = sum / 16; 					// Averaging adc values
		TIM3_Delay_ms(1000);					// Delay of 1sec
	}
}
