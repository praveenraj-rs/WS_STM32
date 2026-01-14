#include "stm32f411xe.h"

/*
===============================================================================
QUADRATURE ENCODER USING TIM1 (STM32F411) – REGISTER LEVEL
ENCODER TYPE : KY-040 (Mechanical Rotary Encoder)
DECODING     : Encoder Mode 3 (X4 decoding)
PINS USED    : PA8 → TIM1_CH1 (Encoder A)
               PA9 → TIM1_CH2 (Encoder B)

WORKING SUMMARY:
- Hardware timer decodes direction and position
- Timer counter (TIM1_CNT) increments / decrements automatically
- Software converts raw counts into logical steps (0–20)
===============================================================================
*/

/* Logical value derived from encoder movement (user-level variable)
   - Range limited between 0 and 20
   - 'volatile' because it changes continuously based on hardware input */
volatile uint8_t encoder_value = 0;

/* Stores previous timer count value
   Used to calculate movement difference (delta) */
static uint16_t last_cnt = 0;


/*
===============================================================================
GPIO INITIALIZATION FOR ENCODER PINS
===============================================================================
PA8  → TIM1_CH1 → Encoder Channel A
PA9  → TIM1_CH2 → Encoder Channel B
Both pins are configured as:
- Alternate Function
- AF1 (TIM1)
- Internal Pull-up enabled (important for KY-040)
===============================================================================
*/
void GPIO_Encoder_Init(void)
{
    /* Enable GPIOA peripheral clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Set PA8 and PA9 to Alternate Function mode (MODER = 10)
       Each pin uses 2 bits in MODER register */
    GPIOA->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));  // Clear mode bits
    GPIOA->MODER |=  ((2U << (8 * 2)) | (2U << (9 * 2)));  // AF mode

    /* Select Alternate Function 1 (AF1) for PA8 and PA9
       AF1 connects pins to TIM1_CH1 and TIM1_CH2 */
    GPIOA->AFR[1] &= ~((0xF << 0) | (0xF << 4));  // Clear AF bits
    GPIOA->AFR[1] |=  ((1 << 0) | (1 << 4));      // AF1 = TIM1

    /* Enable internal pull-up resistors
       - KY-040 is a mechanical encoder
       - Pull-ups prevent floating inputs and noise */
    GPIOA->PUPDR &= ~((3U << (8 * 2)) | (3U << (9 * 2))); // Clear PUPD
    GPIOA->PUPDR |=  ((1U << (8 * 2)) | (1U << (9 * 2))); // Pull-up
}


/*
===============================================================================
TIMER 1 INITIALIZATION FOR ENCODER INTERFACE
===============================================================================
- Timer works in Encoder Interface Mode
- Counts quadrature signals directly in hardware
- Encoder Mode 3 = X4 decoding (maximum resolution)
===============================================================================
*/
void TIM1_Encoder_Init(void)
{
    /* Enable TIM1 peripheral clock */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Reset control register and counter */
    TIM1->CR1 = 0;
    TIM1->CNT = 0;

    /*
    ---------------------------------------------------------------------------
    ENCODER MODE SELECTION
    ---------------------------------------------------------------------------
    SMS bits in SMCR register:
    001 → Encoder Mode 1 (X2)
    010 → Encoder Mode 2 (X2)
    011 → Encoder Mode 3 (X4)  ← used here

    Mode 3 counts:
    - Rising + Falling edges of Channel A
    - Rising + Falling edges of Channel B
    */
    TIM1->SMCR &= ~TIM_SMCR_SMS;                       // Clear SMS bits
    TIM1->SMCR |=  TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;    // SMS = 011 (Mode 3)

    /*
    ---------------------------------------------------------------------------
    CAPTURE/COMPARE CONFIGURATION
    ---------------------------------------------------------------------------
    CC1 and CC2 are configured as inputs
    CC1 ← TI1 (Channel A)
    CC2 ← TI2 (Channel B)
    */
    TIM1->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S); // Clear CCxS
    TIM1->CCMR1 |=  (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);

    /*
    ---------------------------------------------------------------------------
    INPUT FILTER (DEBOUNCING)
    ---------------------------------------------------------------------------
    ICxF = 6:
    - Sampling frequency = fCK_INT / 32
    - 8 consecutive samples required
    - Strong debounce suitable for mechanical encoders
    */
    TIM1->CCMR1 |= (6 << TIM_CCMR1_IC1F_Pos);  // Filter for Channel A
    TIM1->CCMR1 |= (6 << TIM_CCMR1_IC2F_Pos);  // Filter for Channel B

    /*
    ---------------------------------------------------------------------------
    INPUT POLARITY
    ---------------------------------------------------------------------------
    CCxP = 0 → Rising edge reference
    Polarity still matters for direction interpretation even in X4 mode
    */
    TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);

    /* Set maximum counter range (16-bit) */
    TIM1->ARR = 0xFFFF;

    /* Enable the timer (start encoder counting) */
    TIM1->CR1 |= TIM_CR1_CEN;
}


/*
===============================================================================
MAIN FUNCTION
===============================================================================
- Reads raw encoder counts from TIM1_CNT
- Converts raw X4 counts into logical steps
- Limits value between 0 and 20
===============================================================================
*/
int main(void)
{
    /* Initialize GPIO and Timer for encoder */
    GPIO_Encoder_Init();
    TIM1_Encoder_Init();

    /* Store initial counter value */
    last_cnt = TIM1->CNT;

    while (1)
    {
        /* Read current encoder count */
        uint16_t curr_cnt = TIM1->CNT;

        /* Calculate difference since last read
           Positive → Clockwise
           Negative → Counter-Clockwise */
        int16_t diff = curr_cnt - last_cnt;

        /*
        KY-040 produces:
        - 4 counts per physical click (X4 decoding)

        So:
        +4 → One step forward
        -4 → One step backward
        */
        if (diff >= 4)
        {
            if (encoder_value < 20)
                encoder_value++;

            last_cnt = curr_cnt;  // Update reference
        }
        else if (diff <= -4)
        {
            if (encoder_value > 0)
                encoder_value--;

            last_cnt = curr_cnt;  // Update reference
        }
    }
}
