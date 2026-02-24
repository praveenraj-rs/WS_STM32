#include "stm32f4xx.h"
#include <stdint.h>

#define SysClk 16000000U        // Internal RC oscillator 16MHz
#define Buzzer 8U			  	// PA8 Tim1Ch1 PWM

#define Buzzer_PWM_Freq 1000	// Defualt Buzzer PWM frequency 1KHz
#define Volume 20               // Defualt Volume 20%

void TIM3_Init()
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;			// Enable TIM3 Clock
	uint32_t prescaler = (SysClk / 1000) - 1;	// Timer clock = 1 KHz or 1ms
	TIM3->PSC = prescaler;					    // Pre-scaler update
}

void TIM3_Delay_ms(uint16_t delay_ms)
{
	 TIM3->ARR = delay_ms - 1;   			// Auto-reload 1 second delay (1ms * 999)
	 TIM3->CNT = 0;				            // Reset the counter
	 TIM3->EGR |= (1 << 0);                 // Event generation

	 TIM3->SR &= ~ (1 << 0);  				// Clear update interrupt flag
	 TIM3->CR1 |= (1 << 0); 				// Enable Timer

	 while (!(TIM3->SR & (1 << 0))){} 		// Until UIF NEQ 1
	 TIM3->SR &= ~(1 << 0); 				//UIF is cleared manually
	 TIM3->CR1 &= ~(1 << 0); 				//CEN is cleared
}

void Buzzer_TIM1_PWM_Init(void)
{
  // Enable clocks for GPIOA and TIM1
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

  // Configure Buzzer(PA8) as Alternate Function(AF1)
  GPIOA->MODER &= ~(3U << (Buzzer * 2));	    // 00: Clear register
  GPIOA->MODER |=  (2U << (Buzzer * 2));       	// 10: Alternate function
  GPIOA->OTYPER &= ~(1U << Buzzer);            	// 01: Push-pull
  GPIOA->OSPEEDR |= (3U << (Buzzer * 2));      	// 11: Very high speed
  GPIOA->AFR[1] &= ~(0xFU << 0);			    // 00: Clear register
  GPIOA->AFR[1] |=  (1U << 0);            	    // 01: AF1 TIM1_CH1

  // Timer configuration
  // Timer frequency = sysclk / (PSC+1) / (ARR+1)
  uint32_t prescaler = (SysClk / 1000000) - 1;			    // Timer clock = 1 MHz
  uint32_t period = (1000000 / Buzzer_PWM_Freq) - 1;		// ARR
  TIM1->PSC = prescaler;								    // Pre-scaler update
  TIM1->ARR = period;									    // ARR update
  TIM1->CCR1 = 0;  							                // 0% duty cycle

  // PWM mode 1, pre-load enable
  TIM1->CCMR1 &= ~(7U << 4);			// Clear register
  TIM1->CCMR1 |= (6U << 4);        		// OC1M = PWM mode 1
  TIM1->CCMR1 |= TIM_CCMR1_OC1PE;    	// Pre-load enable

  // Enable CH1 output only (no complementary output)
  TIM1->CCER = 0;			    // Clear register
  TIM1->CCER |= TIM_CCER_CC1E;	// Only main output

  // Dead-time and main output enable
  TIM1->BDTR = 0;			    // Clear register
  TIM1->BDTR |= TIM_BDTR_MOE;	// Enable main output

  // Enable timer
  TIM1->CR1 |= TIM_CR1_ARPE;	// Auto-reload pre-load enable
  TIM1->CR1 |= TIM_CR1_CEN;		// Start timer
}

void Buzzer_TIM1_PWM_SetFrequency(uint32_t freq)
{
    if(freq == 0) return;

    // Timer runs at 1 MHz (after prescaler)
    uint32_t period = (1000000 / freq) - 1;

    TIM1->ARR = period;

    // Keep 50% duty by default (optional)
    TIM1->CCR1 = (period + 1) / 2;

    // Force update event to apply changes immediately
    TIM1->EGR |= TIM_EGR_UG;
}

void Buzzer_TIM1_PWM_SetDutyCycle(uint8_t duty_cycle)
{
  if(duty_cycle > 100) duty_cycle = 100;
  TIM1->CCR1 = ((TIM1->ARR + 1) * duty_cycle) / 100;
}


void Buzzer_TIM1_PWM_PlayTone(uint32_t freq, uint16_t duration_ms, uint8_t volume)
{
    if(volume > 100) volume = 100;

    // Set frequency
    Buzzer_TIM1_PWM_SetFrequency(freq);

    // Set volume (duty cycle)
    Buzzer_TIM1_PWM_SetDutyCycle(volume);

    // Play
    TIM3_Delay_ms(duration_ms);

    // Stop sound
    Buzzer_TIM1_PWM_SetDutyCycle(0);
}

void Buzzer_Beep(void)
{
	Buzzer_TIM1_PWM_PlayTone(800,1000,0);
	Buzzer_TIM1_PWM_PlayTone(800,1000,50);
}

void Buzzer_Ambulance(void)
{
	Buzzer_TIM1_PWM_PlayTone(800,500,30);
	Buzzer_TIM1_PWM_PlayTone(500,500,30);
}

void Buzzer_Starwars(void)
{
	Buzzer_TIM1_PWM_PlayTone(440, 500,Volume);
	Buzzer_TIM1_PWM_PlayTone(440, 500,Volume);
	Buzzer_TIM1_PWM_PlayTone(440, 500,Volume);

	Buzzer_TIM1_PWM_PlayTone(349, 350,Volume);
	Buzzer_TIM1_PWM_PlayTone(523, 150,Volume);
	Buzzer_TIM1_PWM_PlayTone(440, 500,Volume);

	Buzzer_TIM1_PWM_PlayTone(349, 350,Volume);
	Buzzer_TIM1_PWM_PlayTone(523, 150,Volume);
	Buzzer_TIM1_PWM_PlayTone(440, 650,Volume);

	TIM3_Delay_ms(1000);
}

void Buzzer_Mario(void)
{
	Buzzer_TIM1_PWM_PlayTone(660, 150,Volume);
	Buzzer_TIM1_PWM_PlayTone(660, 150,Volume);
	TIM3_Delay_ms(100);
	Buzzer_TIM1_PWM_PlayTone(660, 150,Volume);

	TIM3_Delay_ms(150);
	Buzzer_TIM1_PWM_PlayTone(510, 150,Volume);
	Buzzer_TIM1_PWM_PlayTone(660, 150,Volume);
	TIM3_Delay_ms(150);
	Buzzer_TIM1_PWM_PlayTone(770, 150,Volume);

	TIM3_Delay_ms(300);
	Buzzer_TIM1_PWM_PlayTone(380, 150,Volume);

	TIM3_Delay_ms(500); // Wait and loop
}

int main(void)
{
	// Initialization
	TIM3_Init();						// Timer Dealy initialization
	Buzzer_TIM1_PWM_Init();				// Buzzer PWM initialization

  while(1)
  {
    // Buzzer_Beep();
    // Buzzer_Ambulance();
    // Buzzer_Starwars();
    Buzzer_Mario();
  }
}
