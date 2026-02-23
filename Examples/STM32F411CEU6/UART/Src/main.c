#include "stm32f4xx.h"

#define SysClk 16000000U 	// 16MHz HSI system clock frequency

#define Tx1	9				// PA9 Tx UART1
#define Rx1 10				// PA10 Rx UART1

void TIM3_Init()
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
	 TIM3->CR1 |= (1 << 0); 				// Enable Timer

	 while (!(TIM3->SR & (1 << 0))){} 		// Until UIF NEQ 1
	 TIM3->SR &= ~(1 << 0); 				//UIF is cleared manually
	 TIM3->CR1 &= ~(1 << 0); 				//CEN is cleared
}


void UART1_Init(void)
{
	 // Enable clocks for GPIOA and USART1
	 RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // GPIOA clock enable
	 RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // USART1 clock enable

	 // Configure PA9 (TX1) and PA10 (RX1) as Alternate Function 7 (AF7)
	 GPIOA->MODER &= ~( (3U << (Tx1 * 2)) | (3U << (Rx1 * 2)) );  					// 00: Clear register
	 GPIOA->MODER |=  ( (2U << (Tx1 * 2)) | (2U << (Rx1 * 2)) );  					// 10: AF mode
	 GPIOA->AFR[1] &= ~( (0xFU << ((Tx1 - 8) * 4)) | (0xFU << ((Rx1 - 8) * 4)) );	// 00: Clear register
	 GPIOA->AFR[1] |=  ( (7U << ((Tx1 - 8) * 4)) | (7U << ((Rx1 - 8) * 4)) );  		// AF7 for USART1
	 GPIOA->OSPEEDR |= (3U << (Tx1 * 2)) | (3U << (Rx1 * 2));  						// High speed

	 // Configure USART1
	 USART1->CR1 = 0;  								// Disable before configuration
	 USART1->BRR = 0x0682;		  					// 9600 baud @16 MHz
	 USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);  // Enable TX, RX
	 USART1->CR1 |= USART_CR1_UE;                   // Enable USART1
}

void UART1_Send_Char(char c)
{
	while (!(USART1->SR & USART_SR_TXE));  // wait until TX buffer empty
	USART1->DR = (c & 0xFF);
}

void UART1_Send_Str(char *str)
{
	 while(*str)
	 {
		 UART1_Send_Char(*str++);
	 }
}

char UART1_Receive_Char(void)
{
   while (!(USART1->SR & USART_SR_RXNE));  // wait until data received
   return (char)(USART1->DR & 0xFF);
}

void UART1_Receive_Str(char *buffer)
{
   char c;
   uint16_t i = 0;
   do {
       c = UART1_Receive_Char();
       buffer[i++] = c;
   } while (c != '\n' && c != '\r');  	// until newline or carriage return
   buffer[i] = '\0';  					// null terminate
}


int main(void)
{
	TIM3_Init();
	UART1_Init();	// UART1 initialization
	
	while (1)
	{
		UART1_Send_Str("Hello, World!");
		TIM3_Delay_ms(2000);			// Delay
	}
}
