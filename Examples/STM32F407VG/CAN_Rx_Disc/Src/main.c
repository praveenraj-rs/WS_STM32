//RX
#include "stm32f4xx.h"

#define SYS_CLK 16000000U

#define MCP_RESET 0xC0
#define MCP_READ  0x03
#define MCP_WRITE 0x02

/* ================= TIM3 DELAY ================= */

void TIM3_Delay_ms(uint16_t ms)
{
RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

TIM3->PSC = (SYS_CLK/1000)-1;
TIM3->ARR = ms-1;
TIM3->CNT = 0;

TIM3->SR &= ~TIM_SR_UIF;
TIM3->CR1 |= TIM_CR1_CEN;

while(!(TIM3->SR & TIM_SR_UIF));

TIM3->CR1 &= ~TIM_CR1_CEN;
TIM3->SR &= ~TIM_SR_UIF;
}

/* ================= UART ================= */

void UART2_Init()
{

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

GPIOA->MODER |= (2<<(2*2))|(2<<(3*2));
GPIOA->AFR[0] |= (7<<(2*4))|(7<<(3*4));

USART2->BRR = 0x0683;

USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

}

void UART_SendChar(char c)
{
while(!(USART2->SR & USART_SR_TXE));
USART2->DR=c;
}

void UART_SendString(char *s)
{
while(*s)
UART_SendChar(*s++);
}

void UART_Print(uint16_t num)
{
UART_SendChar((num/1000)%10+'0');
UART_SendChar((num/100)%10+'0');
UART_SendChar((num/10)%10+'0');
UART_SendChar((num%10)+'0');
}

/* ================= SPI ================= */

void SPI1_Init()
{

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

GPIOA->MODER |= (2<<(5*2))|(2<<(6*2))|(2<<(7*2));
GPIOA->AFR[0] |= (5<<(5*4))|(5<<(6*4))|(5<<(7*4));

GPIOA->MODER |= (1<<(4*2));

SPI1->CR1 = SPI_CR1_MSTR|SPI_CR1_SSM|SPI_CR1_SSI|SPI_CR1_BR_1;
SPI1->CR1 |= SPI_CR1_SPE;

}

uint8_t SPI_TX(uint8_t data)
{

while(!(SPI1->SR & SPI_SR_TXE));
SPI1->DR=data;

while(!(SPI1->SR & SPI_SR_RXNE));
return SPI1->DR;

}

/* ================= MCP2515 ================= */

void CS_LOW(){GPIOA->BSRR=(1<<20);}
void CS_HIGH(){GPIOA->BSRR=(1<<4);}

void MCP_Write(uint8_t addr,uint8_t data)
{
CS_LOW();
SPI_TX(MCP_WRITE);
SPI_TX(addr);
SPI_TX(data);
CS_HIGH();
}

uint8_t MCP_Read(uint8_t addr)
{

uint8_t val;

CS_LOW();

SPI_TX(MCP_READ);
SPI_TX(addr);

val=SPI_TX(0);

CS_HIGH();

return val;

}

void MCP_Init()
{

CS_LOW();
SPI_TX(MCP_RESET);
CS_HIGH();

MCP_Write(0x0F,0x80);

MCP_Write(0x2A,0x00);
MCP_Write(0x29,0x90);
MCP_Write(0x28,0x02);

MCP_Write(0x60,0x60);

MCP_Write(0x0F,0x00);

}

/* ================= CAN RECEIVE ================= */

uint16_t CAN_Read()
{

uint8_t l,h;

if(MCP_Read(0x2C)&0x01)
{
l=MCP_Read(0x66);
h=MCP_Read(0x67);

MCP_Write(0x2C,0x00);

return (h<<8)|l;
}

return 0xFFFF;

}

/* ================= LED ================= */

void LED_Init()
{

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

GPIOB->MODER |= (1<<(0*2));
GPIOB->MODER |= (1<<(1*2));

}

/* ================= PWM ================= */

void PWM_Init()
{

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

/* PB6 TIM4 CH1 */

GPIOB->MODER |= (2<<(6*2));
GPIOB->AFR[0] |= (2<<(6*4));

TIM4->PSC = 16-1;
TIM4->ARR = 1000;

TIM4->CCMR1 |= (6<<4);
TIM4->CCMR1 |= TIM_CCMR1_OC1PE;

TIM4->CCER |= TIM_CCER_CC1E;

TIM4->CR1 |= TIM_CR1_CEN;

}

void PWM_Set(uint16_t duty)
{
TIM4->CCR1 = duty;
}

/* ================= MAIN ================= */

int main()
{

SPI1_Init();
UART2_Init();
MCP_Init();
LED_Init();
PWM_Init();

UART_SendString("Receiver Ready\r\n");

while(1)
{

uint16_t val;

val = CAN_Read();

if(val != 0xFFFF)
{

UART_SendString("ADC : ");
UART_Print(val);
UART_SendString("\r\n");

/* Linear PWM */

uint16_t pwm = (val*1000)/4095;

PWM_Set(pwm);

/* LED logic */

if(val < 2500)
{
GPIOB->BSRR = (1<<0);
GPIOB->BSRR = (1<<(1+16));
}
else
{
GPIOB->BSRR = (1<<1);
GPIOB->BSRR = (1<<(0+16));
}

}

TIM3_Delay_ms(300);

}

}
