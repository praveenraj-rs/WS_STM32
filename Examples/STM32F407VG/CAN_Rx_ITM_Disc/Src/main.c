#include "stm32f4xx.h"

#define SYS_CLK 16000000U

#define MCP_RESET 0xC0
#define MCP_READ  0x03
#define MCP_WRITE 0x02

#define LED_GREEN 12 // PD12 User Green LED
#define LED_RED   14 // PD14 User Red LED


/* ================= ITM DEBUG ================= */

void ITM_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    ITM->LAR = 0xC5ACCE55;

    ITM->TCR = ITM_TCR_ITMENA_Msk |
               ITM_TCR_SYNCENA_Msk |
               ITM_TCR_TSENA_Msk;

    ITM->TER = 1;
}

void ITM_SendString(char *str)
{
    while(*str)
    {
        ITM_SendChar(*str++);
    }
}

void ITM_Print(uint16_t num)
{
    ITM_SendChar((num/1000)%10+'0');
    ITM_SendChar((num/100)%10+'0');
    ITM_SendChar((num/10)%10+'0');
    ITM_SendChar((num%10)+'0');
}


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
}


/* ================= SPI ================= */

void SPI1_Init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOA->MODER &= ~((3<<(5*2))|(3<<(6*2))|(3<<(7*2)));
    GPIOA->MODER |=  ((2<<(5*2))|(2<<(6*2))|(2<<(7*2)));

    GPIOA->AFR[0] |= (5<<(5*4))|(5<<(6*4))|(5<<(7*4));

    GPIOA->MODER |= (1<<(4*2)); // CS pin output

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI_TX(uint8_t data)
{
    while(!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;

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
    val = SPI_TX(0);
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
        l = MCP_Read(0x66);
        h = MCP_Read(0x67);

        MCP_Write(0x2C,0x00);

        return (h<<8)|l;
    }

    return 0xFFFF;
}

/* ================= DISCOVERY LED ================= */

void DISC_LED_Init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    GPIOD->MODER &= ~((3<<(LED_GREEN*2))|(3<<(LED_RED*2)));
    GPIOD->MODER |=  ((1<<(LED_GREEN*2))|(1<<(LED_RED*2)));

    GPIOD->OTYPER &= ~((1<<LED_GREEN)|(1<<LED_RED));
}

void DISC_LED_Update(uint16_t adc)
{
    if(adc < 2048)
    {
        GPIOD->BSRR = (1<<LED_GREEN);
        GPIOD->BSRR = (1<<(LED_RED+16));
    }
    else
    {
        GPIOD->BSRR = (1<<LED_RED);
        GPIOD->BSRR = (1<<(LED_GREEN+16));
    }
}


/* ================= MAIN ================= */

int main()
{

	SPI1_Init();
	ITM_Init();
	MCP_Init();
	DISC_LED_Init();

	ITM_SendString("Receiver Ready\n");

	while(1)
	{

		uint16_t val;
		val = CAN_Read();

		if(val != 0xFFFF)
		{
			ITM_SendString("ADC : ");
			ITM_Print(val);
			ITM_SendString("\n");

			DISC_LED_Update(val);
		}
			TIM3_Delay_ms(300);
	}
}
