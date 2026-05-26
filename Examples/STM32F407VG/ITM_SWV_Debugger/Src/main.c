#include "stm32f4xx.h"

void ITM_SendString(char *str)
{
    while(*str)
    {
        ITM_SendChar(*str++);
    }
}

void delay()
{
    for(volatile int i=0;i<2000000;i++);
}

int main(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    ITM->LAR = 0xC5ACCE55;

    ITM->TCR = ITM_TCR_ITMENA_Msk |
               ITM_TCR_SYNCENA_Msk |
               ITM_TCR_TSENA_Msk;

    ITM->TER = 1;

    while(1)
    {
        ITM_SendString("Hello STM32F407\n");
        delay();
    }
}
