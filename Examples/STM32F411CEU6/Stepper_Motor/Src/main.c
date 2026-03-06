// Stepper Motor Control

#include "stm32f411xe.h"

#define SysClk 16000000U 	// 16MHz HSI system clock frequency

// Stepper motor (28BYJ48) control with ULN2003 driver
#define In1 12 // PB12
#define In2 13 // PB13
#define In3 14 // PB14
#define In4 15 // PB15

int Direction = 0; 		// 0-Clockwise, 1-Anti-Clockwise
int Step_delay_ms = 4; 	// Delay inbetween the delay

uint8_t step_seq[8] =
{
	0b1000, // 8
	0b1100, // 12
	0b0100,	// 4
	0b0110, // 6
	0b0010,	// 2
	0b0011,	// 3
	0b0001,	// 1
	0b1001	// 9
};

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

void Stepper_GPIO_Init(void)
{
    // 1. Enable GPIOB clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // 2. Clear mode bits (2 bits per pin)
    GPIOB->MODER &= ~(
            (3U << (In1 * 2)) |
            (3U << (In2 * 2)) |
            (3U << (In3 * 2)) |
            (3U << (In4 * 2))
    );

    // 3. Set as General Purpose Output (01)
    GPIOB->MODER |= (
            (1U << (In1 * 2)) |
            (1U << (In2 * 2)) |
            (1U << (In3 * 2)) |
            (1U << (In4 * 2))
    );

    // 4. Output type = Push-Pull
    GPIOB->OTYPER &= ~(
            (1U << In1) |
            (1U << In2) |
            (1U << In3) |
            (1U << In4)
    );

    // 5. Optional: Set High Speed
    GPIOB->OSPEEDR |= (
            (3U << (In1 * 2)) |
            (3U << (In2 * 2)) |
            (3U << (In3 * 2)) |
            (3U << (In4 * 2))
    );

    // 6. No pull-up, no pull-down
    GPIOB->PUPDR &= ~(
            (3U << (In1 * 2)) |
            (3U << (In2 * 2)) |
            (3U << (In3 * 2)) |
            (3U << (In4 * 2))
    );
}

void Stepper_GPIO_Half_Step(void)
{
	if (Direction)
	{
		for(int i = 0; i<8; i++)
		{
			GPIOB->ODR &= ~(0xF << 12);      // Clear PB12–PB15
			GPIOB->ODR |= step_seq[i];       // Output step
			TIM3_Delay_ms(Step_delay_ms);
		}
	}
	else
	{
		for(int i = 8; i>0; i--)
		{
			GPIOB->ODR &= ~(0xF << 12);      // Clear PB12–PB15
			GPIOB->ODR |= step_seq[i];       // Output step
			TIM3_Delay_ms(Step_delay_ms);
		}
	}
}

void Stepper_GPIO_Full_Step(void)
{
	if (Direction)
	{
		for(int i = 0; i<8; i+=2)
		{
			GPIOB->ODR &= ~(0xF << 12);      // Clear PB12–PB15
			GPIOB->ODR |= step_seq[i];       // Output step
			TIM3_Delay_ms(Step_delay_ms);
		}
	}
	else
	{
		for(int i = 8; i>0; i-=2)
		{
			GPIOB->ODR &= ~(0xF << 12);      // Clear PB12–PB15
			GPIOB->ODR |= step_seq[i];       // Output step
			TIM3_Delay_ms(Step_delay_ms);
		}
	}
}

int main(void)
{
	TIM3_Delay_Init();
	Stepper_GPIO_Init();

    while(1)
    {
    	Stepper_GPIO_Full_Step();
    }
}
