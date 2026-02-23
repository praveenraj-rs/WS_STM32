#include "stm32f4xx.h"
#include <stdint.h>

/* ================= CONFIG ================= */
#define SYSCLK_HZ           16000000UL
#define PULSES_PER_REV      8U
#define RPM_WINDOW_MS       1000U     // RPM update rate (100 ms)

/* ================= GLOBALS ================= */
volatile uint32_t system_ms = 0;
volatile uint32_t pulse_count = 0;
volatile uint32_t last_valid_pulse_time = 0;

volatile uint32_t rpm_display = 0;

/* ================= TIM2: 1ms SYSTEM TICK ================= */
static void TIM2_Init_1ms(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 16MHz / 1600 = 10kHz
    TIM2->PSC = 1600 - 1;

    // 10 counts of 10kHz = 1ms
    TIM2->ARR = 10 - 1;

    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        system_ms++;
    }
}

/* ================= RPM INPUT: EXTI PA6 ================= */
static void RPM_EXTI_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA6 as input
    GPIOA->MODER &= ~(3U << (6 * 2));
    GPIOA->PUPDR &= ~(3U << (6 * 2));
    GPIOA->PUPDR |=  (1U << (6 * 2));   // Pull-down

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // EXTI6 mapped to PA6
    SYSCFG->EXTICR[1] &= ~(0xF << 8);

    EXTI->IMR  |= (1U << 6);
    EXTI->RTSR |= (1U << 6);            // Rising edge

    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & (1U << 6)) {
        EXTI->PR = (1U << 6);

        uint32_t now = system_ms;

        // 1ms debounce / glitch rejection
        if ((now - last_valid_pulse_time) >= 1) {
            pulse_count++;
            last_valid_pulse_time = now;
        }
    }
}

/* ================= MAIN ================= */
int main(void)
{
    TIM2_Init_1ms();
    RPM_EXTI_Init();

    uint32_t last_loop_ms = system_ms;
    uint32_t rpm_timer_ms = 0;
    uint32_t last_rpm_pulses = 0;

    while (1)
    {
        uint32_t now = system_ms;
        uint32_t dt  = now - last_loop_ms;

        if (dt < 5) continue;   // small pacing
        last_loop_ms = now;

        rpm_timer_ms += dt;

        if (rpm_timer_ms >= RPM_WINDOW_MS)
        {
            /* ---- SAFE READ OF SHARED VARIABLE ---- */
            __disable_irq();
            uint32_t p_now = pulse_count;
            __enable_irq();

            uint32_t p_delta = p_now - last_rpm_pulses;
            last_rpm_pulses = p_now;

            uint32_t raw_rpm = 0;

            if (p_delta > 0) {
                raw_rpm = (uint32_t)(
                    ((uint64_t)p_delta * 60000ULL) /
                    (PULSES_PER_REV * rpm_timer_ms)
                );
            }

            // Noise cleanup
            if (raw_rpm < 3) raw_rpm = 0;
            if (raw_rpm > 6000) raw_rpm = 6000;

            // Light low-pass filter
            rpm_display = (rpm_display * 7 + raw_rpm * 3) / 100;

            rpm_timer_ms = 0;
        }

        /*
         * rpm_display now contains final RPM value
         * (Use it for LCD / UART / CAN / control loop)
         */
    }
}
