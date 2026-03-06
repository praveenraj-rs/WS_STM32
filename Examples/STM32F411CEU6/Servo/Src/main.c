// Servo Control

#include "stm32f4xx.h"
#include <stdint.h>

#define SysClk 16000000U 	// 16MHz HSI system clock frequency

#define Servo 15U 			// PA15 Tim2Ch1 PWM
#define Servo_PWM_Freq 50	// 50Hz servo control frequency

uint8_t target_angle = 180;

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

void Servo_TIM2_PWM_Init(void)
{
  // Enable clocks for GPIOA and TIM1
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // Configure Servo(PA15) as Alternate Function(AF1)
  GPIOA->MODER &= ~(3U << (Servo * 2));			// Clear register
  GPIOA->MODER |=  (2U << (Servo * 2));       	// Alternate function
  GPIOA->OTYPER &= ~(1U << Servo);            	// Push-pull
  GPIOA->OSPEEDR |= (3U << (Servo * 2));      	// Very high speed
  GPIOA->AFR[1] &= ~(0xFU << (4*7));			// Clear register
  GPIOA->AFR[1] |=  (1U << (4*7));            	// AF1 TIM2_CH1

  // Timer configuration
  // Timer frequency = sysclk / (PSC+1) / (ARR+1)
  uint32_t prescaler = (SysClk / 1000000) - 1;			// Timer clock = 1MHz
  uint32_t period = (1000000 / Servo_PWM_Freq) - 1;		// ARR
  TIM2->PSC = prescaler;								// Pre-scaler update
  TIM2->ARR = period;									// ARR update
  TIM2->CCR1 = period / 2;  							// default 50% duty cycle

  // PWM mode 1, pre-load enable
  TIM2->CCMR1 &= ~(7U << 4);			// Clear register
  TIM2->CCMR1 |= (6U << 4);        		// OC1M = PWM mode 1
  TIM2->CCMR1 |= TIM_CCMR1_OC1PE;    	// Pre-load enable

  // Enable CH1 output only (no complementary output)
  TIM2->CCER = 0;				// Clear register
  TIM2->CCER |= TIM_CCER_CC1E;	// Only main output
  // Enable timer
  TIM2->CR1 |= TIM_CR1_ARPE;	// Auto-reload pre-load enable
  TIM2->CR1 |= TIM_CR1_CEN;		// Start timer
}

void Servo_TIM2_PWM_SetDutyCycle(uint8_t duty_cycle)
{
	if(duty_cycle > 100) duty_cycle = 100;
	TIM2->CCR1 = ((TIM2->ARR + 1) * duty_cycle) / 100;
}

void Servo_TIM2_PWM_SetAngle(uint8_t angle)
{

//	#define MIN_PULSE_WIDTH       544     // the shortest pulse sent to a servo
//	#define MAX_PULSE_WIDTH      2400     // the longest pulse sent to a servo
//	#define DEFAULT_PULSE_WIDTH  1500     // default pulse width when servo is attached
//	#define REFRESH_INTERVAL    20000     // minimum time to refresh servos in microseconds

	if(angle < 0) 	angle = 0;
	if(angle > 180) angle = 180;

	// CCR =  MIN_PULSE_WIDTH + ((MAX_PULSE_WIDTH - MIN_PULSE_WIDTH)/180)*angle
	// TIM2->CCR1 = 544 + (10.312*angle);
	 TIM2->CCR1 = 544 + (((2400-544)/180)*angle);
}

int main(void)
{
	// Initialization
	TIM3_Delay_Init();					// Timer3 delay initialization
	Servo_TIM2_PWM_Init();				// Motor PWM initialization
	while(1)
	{
		for (int angle = 0 ; angle<=target_angle; angle+=30)
		{
			Servo_TIM2_PWM_SetAngle(angle);
			TIM3_Delay_ms(50);
		}

		TIM3_Delay_ms(500);

		for (int angle = target_angle; angle>=0; angle-=30)
		{
			Servo_TIM2_PWM_SetAngle(angle);
			TIM3_Delay_ms(50);
		}

		TIM3_Delay_ms(500);
	}
}
