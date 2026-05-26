#include "stm32f4xx.h"
#include "uart.h"
#include "timer.h"

int main(void)
{
    UART2_Init();
    TIM3_Delay_Init();

    while(1)
    {
        UART2_SendString("Hello World\r\n");
        TIM3_Delay_ms(1000);
    }
}
