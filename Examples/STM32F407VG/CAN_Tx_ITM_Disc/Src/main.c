//TX
#include "stm32f4xx.h"

#define SYS_CLK 16000000U

#define MCP_RESET 0xC0
#define MCP_WRITE 0x02
#define MCP_RTS   0x81

/* ================= TIM3 DELAY ================= */
void TIM3_Delay_ms(uint16_t ms)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = (SYS_CLK / 1000) - 1;
    TIM3->ARR = ms - 1;
    TIM3->CNT = 0;

    TIM3->SR &= ~TIM_SR_UIF;
    TIM3->CR1 |= TIM_CR1_CEN;

    while (!(TIM3->SR & TIM_SR_UIF));

    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM3->SR &= ~TIM_SR_UIF;
}

/* ================= UART ================= */

void UART2_Init()
{
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

GPIOA->MODER |= (2<<(2*2)) | (2<<(3*2));
GPIOA->AFR[0] |= (7<<(2*4)) | (7<<(3*4));

USART2->BRR = 0x0683;

USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART_SendChar(char c)
{
while(!(USART2->SR & USART_SR_TXE));
USART2->DR = c;
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

void MCP_Init()
{
CS_LOW();
SPI_TX(MCP_RESET);
CS_HIGH();

MCP_Write(0x0F,0x80);

MCP_Write(0x2A,0x00);
MCP_Write(0x29,0x90);
MCP_Write(0x28,0x02);

MCP_Write(0x0F,0x00);
}

/* ================= ADC ================= */

void ADC1_Init()
{
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

GPIOA->MODER |= (3<<(1*2));

ADC1->SQR3 = 1;
ADC1->CR2 |= ADC_CR2_ADON;
}

uint16_t ADC_Read()
{
ADC1->CR2 |= ADC_CR2_SWSTART;

while(!(ADC1->SR & ADC_SR_EOC));

return ADC1->DR;
}

/* ================= CAN SEND ================= */

void CAN_Send(uint16_t value)
{
MCP_Write(0x31,0x20);
MCP_Write(0x32,0x00);

MCP_Write(0x35,2);

MCP_Write(0x36,value & 0xFF);
MCP_Write(0x37,value >> 8);

CS_LOW();
SPI_TX(MCP_RTS);
CS_HIGH();
}

/* ================= MAIN ================= */

int main()
{

SPI1_Init();
UART2_Init();
ADC1_Init();
MCP_Init();

UART_SendString("Transmitter Started\r\n");
UART_SendString("--------------------\r\n");

while(1)
{
uint16_t adc;

adc=ADC_Read();

UART_SendString("ADC Value : ");
UART_Print(adc);
UART_SendString("\r\n");

CAN_Send(adc);

TIM3_Delay_ms(200);
}

}










