#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define HXT_STATUS 1<<0
#define PLL_STATUS 1<<2
#define TEAM_NAME "Team 08"

void System_Config(void);
void SPI3_Config(void);
void ADC7_Config(void);

void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void SPI3_SendByte(uint8_t data);

int main(void)
{
    uint32_t adc7_val;
    char adc7_val_s[5] = "0000";

    System_Config();
    SPI3_Config();
    ADC7_Config();

    LCD_start();
    LCD_clear();

    // LCD static content
    printS_5x7(2, 0, "EEET2481 Lab 7 ADC");
    printS_5x7(2, 8, "ADC7 conversion test");
    printS_5x7(2, 16, "Reference voltage: 3.3 V");
    printS_5x7(2, 24, "A/D resolution: 0.805 mV");
    printS_5x7(2, 40, "A/D value:");

    ADC->ADCR |= (1 << 11); // start ADC channel 7 conversion

    while (1) {
    while (!(ADC->ADSR & (1 << 0))); // Wait until conversion is completed (ADF=1)
    ADC->ADSR |= (1 << 0); // Write 1 to clear ADF
    adc7_val = ADC->ADDR[7] & 0x0000FFFF;
    sprintf(adc7_val_s, "%d", adc7_val);
    printS_5x7(4 + 5 * 10, 40, "    ");
    printS_5x7(4 + 5 * 10, 40, adc7_val_s);

    // Calculate the voltage
    float voltage = (adc7_val * 3.3) / 4095;

    // Check if voltage is above 2V
    if (voltage > 2.0) {
        const char* name = TEAM_NAME;
        while (*name) {
            SPI3_SendByte(*name++);
        }
    }

    CLK_SysTickDelay(200000); // Adjusted delay for quicker response
}
//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------

void LCD_start(void)
{
    LCD_command(0xE2); // Set system reset
    LCD_command(0xA1); // Set Frame rate 100 fps  
    LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)  
    LCD_command(0x81); // Set V BIAS potentiometer
    LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()           
    LCD_command(0xC0);
    LCD_command(0xAF); // Set Display Enable
}

void LCD_command(unsigned char temp)
{
    SPI3->SSR |= 1 << 0;
    SPI3->TX[0] = temp;
    SPI3->CNTRL |= 1 << 0;
    while (SPI3->CNTRL & (1 << 0));
    SPI3->SSR &= ~(1 << 0);
}

void LCD_data(unsigned char temp)
{
    SPI3->SSR |= 1 << 0;
    SPI3->TX[0] = 0x0100 + temp;
    SPI3->CNTRL |= 1 << 0;
    while (SPI3->CNTRL & (1 << 0));
    SPI3->SSR &= ~(1 << 0);
}

void LCD_clear(void)
{
    int16_t i;
    LCD_SetAddress(0x0, 0x0);
    for (i = 0; i < 132 * 8; i++)
    {
        LCD_data(0x00);
    }
}

void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr)
{
    LCD_command(0xB0 | PageAddr);
    LCD_command(0x10 | (ColumnAddr >> 4) & 0xF);
    LCD_command(0x00 | (ColumnAddr & 0xF));
}

void System_Config(void) {
    SYS_UnlockReg(); // Unlock protected registers
    
    CLK->PWRCON |= (1 << 0);
  while(!(CLK->CLKSTATUS & HXT_STATUS));
    //PLL configuration starts
    CLK->PLLCON &= ~(1 << 19); //0: PLL input is HXT
    CLK->PLLCON &= ~(1 << 16); //PLL in normal mode
    CLK->PLLCON &= (~(0x01FF << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1 << 18); //0: enable PLLOUT
    while(!(CLK->CLKSTATUS & PLL_STATUS));
    //PLL configuration ends
    
    //clock source selection
    CLK->CLKSEL0 &= (~(0x07 << 0));
    CLK->CLKSEL0 |= (0x02 << 0);
    //clock frequency division
    CLK->CLKDIV &= (~0x0F << 0);
    
    // SPI3 clock enable
    CLK->APBCLK |= 1 << 15;
    
    //ADC Clock selection and configuration
    CLK->CLKSEL1 &= ~(0x03 << 2); // ADC clock source is 12 MHz
    CLK->CLKDIV &= ~(0x0FF << 16);
    CLK->CLKDIV |= (0x0B << 16); // ADC clock divider is (11+1) --> ADC clock is 12/12 = 1 MHz
    CLK->APBCLK |= (0x01 << 28); // enable ADC clock
    
    SYS_LockReg();  // Lock protected registers    
}

void SPI3_Config(void) {
    SYS->GPD_MFP |= 1 << 11; //1: PD11 is configured for SPI3
    SYS->GPD_MFP |= 1 << 9; //1: PD9 is configured for SPI3
    SYS->GPD_MFP |= 1 << 8; //1: PD8 is configured for SPI3

    SPI3->CNTRL &= ~(1 << 23); //0: disable variable clock feature
    SPI3->CNTRL &= ~(1 << 22); //0: disable two bits transfer mode
    SPI3->CNTRL &= ~(1 << 18); //0: select Master mode
    SPI3->CNTRL &= ~(1 << 17); //0: disable SPI interrupt    
    SPI3->CNTRL |= 1 << 11; //1: SPI clock idle high 
    SPI3->CNTRL &= ~(1 << 10); //0: MSB is sent first   
    SPI3->CNTRL &= ~(3 << 8); //00: one transmit/receive word will be executed in one data transfer

    SPI3->CNTRL &= ~(31 << 3); //Transmit/Receive bit length
    SPI3->CNTRL |= 9 << 3;     //9: 9 bits transmitted/received per data transfer

    SPI3->CNTRL |= (1 << 2);  //1: Transmit at negative edge of SPI CLK       
    SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVID-ER+1)*2). HCLK = 50 MHz
}

void ADC7_Config(void) {
    PA->PMD &= ~(0b11 << 14);
    PA->PMD |= (0b01 << 14); // PA.7 is input pin
    PA->OFFD |= (0x01 << 7); // PA.7 digital input path is disabled
    SYS->GPA_MFP |= (1 << 7); // GPA_MFP[7] = 1 for ADC7
    SYS->ALT_MFP &= ~(1 << 11); //ALT_MFP[11] = 0 for ADC7

    //ADC operation configuration
    ADC->ADCR |= (0b11 << 2);  // continuous scan mode
    ADC->ADCR &= ~(1 << 1); // ADC interrupt is disabled
    ADC->ADCR |= (0x01 << 0); // ADC is enabled
    ADC->ADCHER &= ~(0b11 << 8); // ADC7 input source is external pin
    ADC->ADCHER |= (1 << 7); // ADC channel 7 is enabled.
}

void SPI3_SendByte(uint8_t data) {
    SPI3->SSR |= 1 << 0;
    SPI3->TX[0] = data;
    SPI3->CNTRL |= 1 << 0;
    while (SPI3->CNTRL & (1 << 0));
    SPI3->SSR &= ~(1 << 0);
}
