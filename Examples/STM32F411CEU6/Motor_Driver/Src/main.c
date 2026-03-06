// Motor Driver Control L293D

#include "stm32f4xx.h"
#include <stdint.h>

#define SysClk 16000000U 	// 16MHz HSI system clock frequency

#define Motor 8U			// PA8 Tim1Ch1 PWM Speed Control
#define Motor_PWM_Freq 1000	// 1KHz motor PWM frequency

#define Motor_DC1	12		// PB12 Motor Direction Control
#define Motor_DC2	13		// PB13 Motor Direction Control

const uint8_t Motor_Direction = 1;	// 0-stop, 1-clockwise, 2-anti-clockwise
const uint8_t Motor_Speed = 50;		// 50% speed (dutycycle)

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

void Motor_TIM1_PWM_Init(void)
{
  // Enable clocks for GPIOA and TIM1
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

  // Configure Motor(PA8) as Alternate Function(AF1)
  GPIOA->MODER &= ~(3U << (Motor * 2));			// 00: Clear register
  GPIOA->MODER |=  (2U << (Motor * 2));       	// 10: Alternate function
  GPIOA->OTYPER &= ~(1U << Motor);            	// 01: Push-pull
  GPIOA->OSPEEDR |= (3U << (Motor * 2));      	// 11: Very high speed
  GPIOA->AFR[1] &= ~(0xFU << 0);				// 00: Clear register
  GPIOA->AFR[1] |=  (1U << 0);            		// 01: AF1 TIM1_CH1

  // Timer configuration
  // Timer frequency = sysclk / (PSC+1) / (ARR+1)
  uint32_t prescaler = (SysClk / 1000000) - 1;			// Timer clock = 1 MHz
  uint32_t period = (1000000 / Motor_PWM_Freq) - 1;		// ARR
  TIM1->PSC = prescaler;								// Pre-scaler update
  TIM1->ARR = period;									// ARR update
  TIM1->CCR1 = 0;  										// 0% duty cycle

  // PWM mode 1, pre-load enable
  TIM1->CCMR1 &= ~(7U << 4);			// Clear register
  TIM1->CCMR1 |= (6U << 4);        		// OC1M = PWM mode 1
  TIM1->CCMR1 |= TIM_CCMR1_OC1PE;    	// Pre-load enable

  // Enable CH1 output only (no complementary output)
  TIM1->CCER = 0;				// Clear register
  TIM1->CCER |= TIM_CCER_CC1E;	// Only main output

  // Dead-time and main output enable
  TIM1->BDTR = 0;				// Clear register
  TIM1->BDTR |= TIM_BDTR_MOE;	// Enable main output

  // Enable timer
  TIM1->CR1 |= TIM_CR1_ARPE;	// Auto-reload pre-load enable
  TIM1->CR1 |= TIM_CR1_CEN;		// Start timer
}

void Motor_TIM1_PWM_SetDutyCycle(uint8_t duty_cycle)
{
  if(duty_cycle > 100) duty_cycle = 100;
  TIM1->CCR1 = ((TIM1->ARR + 1) * duty_cycle) / 100;
}

void Motor_Direction_Control_Init(void)
{
	// Motor_DC1 - PB12, Motor_DC2 - PB13
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; 	// Enable GPIOB clock

	GPIOB->MODER &= ~(3U << (Motor_DC1 * 2));	// 00: Clear register
	GPIOB->MODER &= ~(3U << (Motor_DC2 * 2));	// 00: Clear register

	GPIOB->MODER |=  (1U << (Motor_DC1 * 2));  	// 01: Output mode
	GPIOB->MODER |=  (1U << (Motor_DC2 * 2));  	// 01: Output mode

	GPIOB->OTYPER &= ~(1U << Motor_DC1);		// Push-pull
	GPIOB->OTYPER &= ~(1U << Motor_DC2);		// Push-pull

	GPIOB->PUPDR &= ~(3U << (Motor_DC1 * 2));	// No pull-up or pull-down
	GPIOB->PUPDR &= ~(3U << (Motor_DC2 * 2));	// No pull-up or pull-down

	// Motor in OFF Condition
	// PB12 - Low && PB13 - Low
	GPIOB->ODR &= ~(1<<Motor_DC1);				// Motor_DC1 low
	GPIOB->ODR &= ~(1<<Motor_DC2);				// Motor_DC2 low
}


void Motor_Direction_Control(uint8_t Direction)
{
	// Directions
	// 0 = Stop
	// 1 = Forward
	// 2 = Backward

	if (Direction==0)
	{
		GPIOB->ODR &= ~(1<<Motor_DC1);				// Motor_DC1 low
		GPIOB->ODR &= ~(1<<Motor_DC2);				// Motor_DC2 low
	}

	else if (Direction==1)
	{
		GPIOB->ODR |= (1<<Motor_DC1);				// Motor_DC1 high
		GPIOB->ODR &= ~(1<<Motor_DC2);				// Motor_DC2 low
	}

	else if (Direction==2)
	{
		GPIOB->ODR &= ~(1<<Motor_DC1);				// Motor_DC1 low
		GPIOB->ODR |= (1<<Motor_DC2);				// Motor_DC2 high
	}
}

int main(void)
{
	// Initialisation
	TIM3_Delay_Init();

	Motor_Direction_Control_Init();
	Motor_TIM1_PWM_Init();

	Motor_Direction_Control(Motor_Direction); 	// Clockwise motion
	Motor_TIM1_PWM_SetDutyCycle(Motor_Speed); 	// 50% duty-cycle

	while(1)
	{
		TIM3_Delay_ms(5000);	// 5sec Delay
	}
}

