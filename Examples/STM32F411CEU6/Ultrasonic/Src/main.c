#include "stm32f411xe.h"
#include <stdint.h>

#define SysClk 16000000U 	// 25MHz HSE system clock frequency
#define TRIG_PIN 12         // PB12 trigger pin
#define ECHO_PIN 13         // PB13 echo pin

volatile uint32_t echo_time_us = 0;
volatile uint32_t distance_cm  = 0;


void TIM2_Ultrasonic_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

    // PSC=15 (16-1) is logical for a 16MHz HSI clock
    TIM2->PSC = (SysClk / 1000) - 1;
    TIM2->ARR = 0xFFFFFFFF; // ARR acts as a max ceiling and won't affect distance
    TIM2->CNT = 0;
    TIM2->CR1 |= TIM_CR1_CEN; // Start Timer
}

void TIM2_Ultrasonic_Delay_us(uint32_t us) {
    TIM2->CNT = 0;
    while (TIM2->CNT < us); // Simple blocking delay
}

void TIM3_Delay_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;		// Enable TIM3 Clock
    uint32_t prescaler = (SysClk / 1000) - 1;	// Timer clock = 1 KHz or 1ms
    TIM3->PSC = prescaler;					// Pre-scaler update
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

void GPIO_Ultrasonic_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB clock

    // PB12 -> TRIG (Output)
    GPIOB->MODER &= ~(3U << (TRIG_PIN * 2));
    GPIOB->MODER |=  (1U << (TRIG_PIN * 2));

    // PB13 -> ECHO (Input) with Pull-Down to stabilize signal
    GPIOB->MODER &= ~(3U << (ECHO_PIN * 2));
    GPIOB->PUPDR &= ~(3U << (ECHO_PIN * 2));
    GPIOB->PUPDR |=  (2U << (ECHO_PIN * 2));
}

/* ================= ULTRASONIC READ ================= */
uint32_t Ultrasonic_Read_us(void) {
    uint32_t timeout;

    // Send 10us Trigger Pulse
    GPIOB->BSRR = (1U << (TRIG_PIN + 16)); // LOW
    TIM2_Ultrasonic_Delay_us(2);
    GPIOB->BSRR = (1U << TRIG_PIN);        // HIGH
    TIM2_Ultrasonic_Delay_us(10);
    GPIOB->BSRR = (1U << (TRIG_PIN + 16)); // LOW

    // Wait for ECHO Rising Edge (with safety timeout)
    timeout = 30000;
    while (!(GPIOB->IDR & (1U << ECHO_PIN))) {
        if (--timeout == 0) return 0;
    }

    // Measure duration of HIGH pulse
    TIM2->CNT = 0;
    while (GPIOB->IDR & (1U << ECHO_PIN)) {
        if (TIM2->CNT > 40000) break; // Hardware max limit check
    }
    return TIM2->CNT;
}

int main(void) {

    TIM2_Ultrasonic_Init();
    TIM3_Delay_Init();
    GPIO_Ultrasonic_Init();

    while (1) {
        echo_time_us = Ultrasonic_Read_us();
        if (echo_time_us > 0) {
            /* * CALIBRATION LOGIC:
             * 28253 / 31 = ~911 ticks per cm.
             * 4979 / 5 = ~995 ticks per cm.
             * We use 925 as a balanced divisor.
             */
            distance_cm = echo_time_us / 925;
        } else {
            distance_cm = 0;
        }
        TIM3_Delay_ms(1000);
    }
}
