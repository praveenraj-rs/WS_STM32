#include "stm32f411xe.h"

/* ================= GLOBALS ================= */
volatile uint8_t encoder_value = 0;   // WATCH THIS (0–20)
static uint16_t last_cnt = 0;

/* ================= GPIO INIT ================= */
void GPIO_Encoder_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA8, PA9 → Alternate Function */
    GPIOA->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOA->MODER |=  ((2U << (8 * 2)) | (2U << (9 * 2)));

    /* AF1 → TIM1 */
    GPIOA->AFR[1] &= ~((0xF << 0) | (0xF << 4));
    GPIOA->AFR[1] |=  ((1 << 0) | (1 << 4));

    /* Internal Pull-ups (MANDATORY for KY-040) */
    GPIOA->PUPDR &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOA->PUPDR |=  ((1U << (8 * 2)) | (1U << (9 * 2)));
}

/* ================= TIM1 ENCODER INIT ================= */
void TIM1_Encoder_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->CR1 = 0;
    TIM1->CNT = 0;

    /* Encoder Mode 3 (x4 decoding) */
    TIM1->SMCR &= ~TIM_SMCR_SMS;
    TIM1->SMCR |=  TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;

    /* CC1 & CC2 as inputs */
    TIM1->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
    TIM1->CCMR1 |=  (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);

    /* Input filter (strong debounce for KY-040) */
    TIM1->CCMR1 |= (6 << TIM_CCMR1_IC1F_Pos);
    TIM1->CCMR1 |= (6 << TIM_CCMR1_IC2F_Pos);

    /* Rising edge */
    TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);

    TIM1->ARR = 0xFFFF;

    TIM1->CR1 |= TIM_CR1_CEN;
}

/* ================= MAIN ================= */
int main(void)
{
    GPIO_Encoder_Init();
    TIM1_Encoder_Init();

    last_cnt = TIM1->CNT;

    while (1)
    {
        uint16_t curr_cnt = TIM1->CNT;
        int16_t diff = curr_cnt - last_cnt;

        /* 4 counts = 1 physical click (KY-040) */
        if (diff >= 4)
        {
            if (encoder_value < 20)
                encoder_value++;

            last_cnt = curr_cnt;
        }
        else if (diff <= -4)
        {
            if (encoder_value > 0)
                encoder_value--;

            last_cnt = curr_cnt;
        }
    }
}
